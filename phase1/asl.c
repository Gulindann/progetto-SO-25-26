#include "./headers/asl.h"
#include "./headers/pcb.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

// Initialize the ASL list and the free semaphore list
void initASL()
{
    INIT_LIST_HEAD(&semdFree_h);
    INIT_LIST_HEAD(&semd_h);

    for (int i = 0; i < MAXPROC; i++)
    {
        semd_table[i].s_key = NULL;
        INIT_LIST_HEAD(&semd_table[i].s_procq); // Initialize process queue
        INIT_LIST_HEAD(&semd_table[i].s_link);  // Initialize list link
        list_add(&semd_table[i].s_link, &semdFree_h);
    }
}

// Helper function to find the semaphore with key semAdd
static semd_t *getSemd(int *semAdd)
{
    semd_t *iter;
    list_for_each_entry(iter, &semd_h, s_link)
    {
        if (iter->s_key == semAdd)
        {
            return iter;
        }
        if (iter->s_key > semAdd)
        {
            return NULL; // The list is ordered, if I exceed the value, it's not there
        }
    }
    return NULL;
}

// Insert the PCB p into the queue of the semaphore with key semAdd
int insertBlocked(int *semAdd, pcb_t *p)
{
    semd_t *sem = getSemd(semAdd);

    // If the semaphore does not exist, create a new one, if possible
    if (sem == NULL)
    {
        if (list_empty(&semdFree_h))
        {
            return TRUE;
        }
        // Take from the head of the free list
        sem = container_of(semdFree_h.next, semd_t, s_link);
        list_del(&sem->s_link);

        // Initialize
        INIT_LIST_HEAD(&sem->s_procq);
        INIT_LIST_HEAD(&sem->s_link);
        sem->s_key = semAdd;

        // Ordered insertion into the ASL
        semd_t *iter;
        int inserted = 0;
        list_for_each_entry(iter, &semd_h, s_link)
        {
            if (iter->s_key > semAdd)
            {
                list_add_tail(&sem->s_link, &iter->s_link);
                inserted = 1;
                break;
            }
        }

        // Insert at the tail if semAdd is the largest or if the list is empty
        if (!inserted)
        {
            list_add_tail(&sem->s_link, &semd_h);
        }
    }

    // Insert the PCB into the semaphore queue
    p->p_semAdd = semAdd;
    list_add_tail(&p->p_list, &sem->s_procq);
    return FALSE;
}

// Remove and return the first process blocked on the semaphore with key semAdd
pcb_t *removeBlocked(int *semAdd)
{

    semd_t *sem = getSemd(semAdd);

    if (sem == NULL)
    {
        return NULL;
    }

    pcb_t *p = removeProcQ(&sem->s_procq);

    // If the semaphore queue is empty after removal, remove the semaphore from the ASL
    if (emptyProcQ(&sem->s_procq))
    {
        list_del(&sem->s_link);
        list_add(&sem->s_link, &semdFree_h);
    }

    return p;
}

// Remove the PCB p from the semaphore process queue
pcb_t *outBlocked(pcb_t *p)
{

    if (p->p_semAdd == NULL)
    {
        return NULL;
    }

    semd_t *sem = getSemd(p->p_semAdd);

    if (sem == NULL)
    {
        return NULL;
    }

    // PCB p is removed from the semaphore queue
    pcb_t *removed = outProcQ(&sem->s_procq, p);

    // If the PCB was removed
    if (removed != NULL)
    {
        // If the semaphore queue is empty after removal, remove the semaphore from the ASL
        if (emptyProcQ(&sem->s_procq))
        {
            list_del(&sem->s_link);
            list_add(&sem->s_link, &semdFree_h);
        }
    }

    return removed;
}

// Return the head PCB in the process queue of the semaphore with key semAdd
pcb_t *headBlocked(int *semAdd)
{

    semd_t *sem = getSemd(semAdd);

    if (sem == NULL)
    {
        return NULL;
    }

    return headProcQ(&sem->s_procq);
}