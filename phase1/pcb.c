#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

// Initialize the free PCB list
void initPcbs()
{
    INIT_LIST_HEAD(&pcbFree_h);
    for (int i = 0; i < MAXPROC; i++)
    {
        freePcb(&pcbFree_table[i]);
    }
}

// Insert the PCB pointed to by p into the free PCB list
void freePcb(pcb_t *p)
{
    list_add(&p->p_list, &pcbFree_h);
}

// Return a pointer to a free PCB (NULL if empty)
pcb_t *allocPcb()
{
    if (list_empty(&pcbFree_h))
    {
        return NULL;
    }
    else
    {
        // Remove from the head of the pcbFree list
        struct pcb_t *p = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(&p->p_list);

        // Initialize ALL fields
        INIT_LIST_HEAD(&p->p_list);
        INIT_LIST_HEAD(&p->p_child);
        INIT_LIST_HEAD(&p->p_sib);

        p->p_parent = NULL;
        p->p_semAdd = NULL;
        p->p_supportStruct = NULL;
        p->p_time = 0;
        p->p_prio = 0;

        // Reset process state
        p->p_s.entry_hi = 0;
        p->p_s.cause = 0;
        p->p_s.status = 0;
        p->p_s.pc_epc = 0;
        p->p_s.mie = 0;
        for (int i = 0; i < STATE_GPR_LEN; i++)
        {
            p->p_s.gpr[i] = 0;
        }

        // Assign PID and increment
        p->p_pid = next_pid++;

        return p;
    }
}

// Initialize the process queue sentinel
void mkEmptyProcQ(struct list_head *head)
{
    INIT_LIST_HEAD(head);
}

// Return TRUE if the queue is empty, FALSE otherwise
int emptyProcQ(struct list_head *head)
{
    return list_empty(head);
}

// Insert p into the process queue ordered by priority
void insertProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *iter;

    // Iterate over the list to find the correct position (descending priority order)
    list_for_each_entry(iter, head, p_list)
    {
        if (p->p_prio > iter->p_prio)
        {
            // Insert p BEFORE the current element (iter)
            // since p has higher priority
            list_add_tail(&p->p_list, &iter->p_list);
            return;
        }
    }
    // If the list is empty, or p has the lowest/equal priority, insert at the tail
    list_add_tail(&p->p_list, head);
}

// Return the head element of the queue without removing it
pcb_t *headProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return NULL;
    }
    return container_of(head->next, pcb_t, p_list);
}

// Remove and return the head element of the queue
pcb_t *removeProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return NULL;
    }
    struct pcb_t *p = container_of(head->next, pcb_t, p_list);
    list_del(&p->p_list);
    return p;
}

// Remove the PCB p from the queue pointed to by head (if present)
pcb_t *outProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *iter;

    // Search for p in the list
    list_for_each_entry(iter, head, p_list)
    {
        if (iter == p)
        {
            list_del(&p->p_list);
            return p;
        }
    }
    return NULL;
}

// Return TRUE if p has no children
int emptyChild(pcb_t *p)
{
    return list_empty(&p->p_child);
}

// Insert p as a child of prnt
void insertChild(pcb_t *prnt, pcb_t *p)
{
    p->p_parent = prnt;
    // Add p to the parent's child queue (at the tail, FIFO convention for siblings)
    list_add_tail(&p->p_sib, &prnt->p_child);
}

// Remove and return the first child of p
pcb_t *removeChild(pcb_t *p)
{
    if (emptyChild(p))
    {
        return NULL;
    }

    struct pcb_t *child = container_of(p->p_child.next, pcb_t, p_sib);
    list_del(&child->p_sib);
    child->p_parent = NULL;

    return child;
}

// Remove p from the parent's child list
pcb_t *outChild(pcb_t *p)
{
    if (p->p_parent == NULL)
    {
        return NULL;
    }

    list_del(&p->p_sib);
    p->p_parent = NULL;

    return p;
}