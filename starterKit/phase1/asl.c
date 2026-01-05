#include "./headers/asl.h"
#include "./headers/pcb.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

// Inizializza la lista ASL e la lista dei semafori liberi
void initASL()
{
    INIT_LIST_HEAD(&semdFree_h);
    INIT_LIST_HEAD(&semd_h);

    for (int i = 0; i < MAXPROC; i++)
    {
        semd_table[i].s_key = NULL;
        INIT_LIST_HEAD(&semd_table[i].s_procq); // Inizializza coda processi
        INIT_LIST_HEAD(&semd_table[i].s_link);  // Inizializza link lista
        list_add(&semd_table[i].s_link, &semdFree_h);
    }
}

// Funzione ausiliaria per trovare il semaforo con chiave semAdd
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
            return NULL; // La lista è ordinata, se supero il valore, non c'è
        }
    }
    return NULL;
}

// Inserisce il PCB p nella coda del semaforo con chiave semAdd
int insertBlocked(int *semAdd, pcb_t *p)
{
    semd_t *sem = getSemd(semAdd);

    // Se il semaforo non esiste, ne creo uno nuovo, se possibile
    if (sem == NULL)
    {
        if (list_empty(&semdFree_h))
        {
            return TRUE;
        }
        // Prendo dalla testa dei liberi
        sem = container_of(semdFree_h.next, semd_t, s_link);
        list_del(&sem->s_link);

        // Inizializzo
        INIT_LIST_HEAD(&sem->s_procq);
        INIT_LIST_HEAD(&sem->s_link);
        sem->s_key = semAdd;

        // Inserimento ordinato nella ASL
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

        // Inserimento in coda se semAdd è il più grande o se la lista è vuota
        if (!inserted)
        {
            list_add_tail(&sem->s_link, &semd_h);
        }
    }

    // Inserisco il PCB nella coda del semaforo
    p->p_semAdd = semAdd;
    list_add_tail(&p->p_list, &sem->s_procq);
    return FALSE;
}

pcb_t *removeBlocked(int *semAdd)
{
    struct list_head *temp;
    struct pcb_t *toRemovePCB;
    if (list_empty(&semd_h))
    {
        return NULL;
    }

    list_for_each(temp, &semd_h)
    {
        struct semd_t *toRemove = container_of(temp, semd_t, s_link);
        if (toRemove->s_key == semAdd)
        {
            toRemovePCB = container_of(toRemove->s_procq.next, pcb_t, p_list);
            list_del(toRemove->s_procq.next);
            if (emptyProcQ(&toRemove->s_procq))
            {
                list_del(&toRemove->s_link);
                list_add(&toRemove->s_link, &semdFree_h);
            }
            return toRemovePCB;
        }
    }
    return NULL;
}

// Rimuove e restituisce il primo processo bloccato sul semaforo con chiave semAdd
pcb_t *removeBlocked(int *semAdd)
{

    semd_t *sem = getSemd(semAdd);

    if (sem == NULL)
    {
        return NULL;
    }

    pcb_t *p = removeProcQ(&sem->s_procq);

    // Se la coda del semaforo è vuota dopo la rimozione, rimuovo il semaforo dalla ASL
    if (emptyProcQ(&sem->s_procq))
    {
        list_del(&sem->s_link);
        list_add(&sem->s_link, &semdFree_h);
    }

    return p;
}

// Rimuove il PCB p dalla coda dei processi del semaforo
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

    // PCB p viene rimosso dalla coda del semaforo
    pcb_t *removed = outProcQ(&sem->s_procq, p);

    // Se il PCB è stato rimosso
    if (removed != NULL)
    {
        // Se la coda del semaforo è vuota dopo la rimozione, rimuovo il semaforo dalla ASL
        if (emptyProcQ(&sem->s_procq))
        {
            list_del(&sem->s_link);
            list_add(&sem->s_link, &semdFree_h);
        }
    }

    return removed;
}

// Restituisce il PCB in testa nella coda dei processi del semaforo con chiave semAdd
pcb_t *headBlocked(int *semAdd)
{

    semd_t *sem = getSemd(semAdd);

    if (sem == NULL)
    {
        return NULL;
    }

    return headProcQ(&sem->s_procq);
}
