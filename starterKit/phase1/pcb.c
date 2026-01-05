#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

// Inizializza la lista dei PCB liberi
void initPcbs()
{
    INIT_LIST_HEAD(&pcbFree_h);
    for (int i = 0; i < MAXPROC; i++)
    {
        freePcb(&pcbFree_table[i]);
    }
}

// Inserisce il PCB puntato da p nella lista dei PCB liberi
void freePcb(pcb_t *p)
{
    list_add(&p->p_list, &pcbFree_h);
}

// Restituisce un puntatore ad un PCB libero (NULL se vuota)
pcb_t *allocPcb()
{
    if (list_empty(&pcbFree_h))
    {
        return NULL;
    }
    else
    {
        // Rimozione dalla testa della lista pcbFree
        struct pcb_t *p = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(&p->p_list);

        // Inizializzazione di TUTTI i campi
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

        // Assegnazione PID e incremento
        p->p_pid = next_pid++;

        return p;
    }
}

// Inizializza la sentinella della coda dei processi
void mkEmptyProcQ(struct list_head *head)
{
    INIT_LIST_HEAD(head);
}

// Restituisce TRUE se la coda è vuota, FALSE altrimenti
int emptyProcQ(struct list_head *head)
{
    return list_empty(head);
}

// Inserisce p nella coda dei processi ordinata per priorità
void insertProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *iter;

    // Scorre la lista per trovare la posizione corretta (ordine decrescente di priorità)
    list_for_each_entry(iter, head, p_list)
    {
        if (p->p_prio > iter->p_prio)
        {
            // Inserisce p PRIMA dell'elemento corrente (iter)
            // poichè p ha priorità maggiore
            list_add_tail(&p->p_list, &iter->p_list);
            return;
        }
    }
    // Se la lista è vuota, o p ha la priorità più bassa/uguale a tutti, va in coda
    list_add_tail(&p->p_list, head);
}

// Restituisce l'elemento di testa della coda senza rimuoverlo
pcb_t *headProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return NULL;
    }
    return container_of(head->next, pcb_t, p_list);
}

// Rimuove e restituisce l'elemento di testa della coda
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

// Rimuove il PCB p dalla coda puntata da head (se presente)
pcb_t *outProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *iter;

    // Cerca p nella lista
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

// Restituisce TRUE se p non ha figli
int emptyChild(pcb_t *p)
{
    return list_empty(&p->p_child);
}

// Inserisce p come figlio di prnt
void insertChild(pcb_t *prnt, pcb_t *p)
{
    p->p_parent = prnt;
    // Aggiunge p alla coda dei figli del padre (in coda, convenzione FIFO per i fratelli)
    list_add_tail(&p->p_sib, &prnt->p_child);
}

// Rimuove e restituisce il primo figlio di p
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

// Rimuove p dalla lista dei figli del padre
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