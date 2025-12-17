#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

void initPcbs()
{

    INIT_LIST_HEAD(&pcbFree_h); // Init della lista dei PCB liberi
    for (int i = 0; i < MAXPROC; i++)
    {
        freePcb(&pcbFree_table[i]); // Aggiunta dei PCB alla lista dei PCB liberi
    }
}

void freePcb(pcb_t *p)
{
    list_add(&p->p_list, &pcbFree_h); // Aggiunta del PCB p alla lista dei PCB liberi
}

// Restituisce un puntatore ad un PCB libero.
pcb_t *allocPcb()
{ //

    if (list_empty(&pcbFree_h))
    {
        return NULL; // NULL se non ci sono PCB liberi
    }
    else
    {

        // Rimozione del primo PCB dalla lista dei PCB liberi
        struct list_head *nodo = pcbFree_h.next;
        list_del(nodo);

        pcb_t *emptyPcb = container_of(nodo, pcb_t, p_list);

        // Reset dei campi del PCB
        emptyPcb->p_parent = NULL;
        emptyPcb->p_semAdd = NULL;
        emptyPcb->p_supportStruct = NULL;
        emptyPcb->p_time = 0;

        emptyPcb->p_s.cause = 0;
        emptyPcb->p_s.entry_hi = 0;
        emptyPcb->p_s.mie = 0;
        emptyPcb->p_s.pc_epc = 0;
        emptyPcb->p_s.status = 0;
        for (int i = 0; i < 32; i++)
        {
            emptyPcb->p_s.gpr[i] = 0;
        }
        emptyPcb->p_prio = 0;

        // Inizializzazione delle liste interne del PCB
        INIT_LIST_HEAD(&emptyPcb->p_list);
        INIT_LIST_HEAD(&emptyPcb->p_child);
        INIT_LIST_HEAD(&emptyPcb->p_sib);

        // Assegnazione del PID e incremento del contatore
        emptyPcb->p_pid = next_pid++;

        return emptyPcb;
    }
}

// Init della coda dei processi
void mkEmptyProcQ(struct list_head *head)
{
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head)
{
    return list_empty(head); // Ritorna 1 se la coda è vuota, 0 altrimenti
}

void insertProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *pos;

    list_for_each_entry(pos, head, p_list)
    {
        if (p->p_prio > pos->p_prio)
        {
            list_add_tail(&p->p_list, &pos->p_list); // Inserimento di p prima di pos
            return;
        }
    }
    list_add_tail(&p->p_list, head); // Se la lista è vuota o p ha la prio piu bassa, inserimento in coda
}

// Ritorna PCB in testa
pcb_t *headProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return NULL;
    }
    return container_of(head->next, pcb_t, p_list);
}

// Rimuove e ritorna il PCB in testa
pcb_t *removeProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return NULL;
    }
    pcb_t *toRemove = container_of(head->next, pcb_t, p_list);
    list_del(&toRemove->p_list);
    return toRemove;
}

// Rimuove e ritorna il PCB p
pcb_t *outProcQ(struct list_head *head, pcb_t *p)
{
    pcb_t *pos;
    list_for_each_entry(pos, head, p_list)
    {
        if (pos == p)
        {
            list_del(&p->p_list);
            return p;
        }
    }
    return NULL; // Ritorna NULL se il PCB non viene trovato
}

int emptyChild(pcb_t *p)
{
    return list_empty(&p->p_child); // Ritorna 1 se p non ha figli, 0 altrimenti
}

void insertChild(pcb_t *prnt, pcb_t *p)
{
    p->p_parent = prnt;
    /* Inserimento di p e fratelli nella lista dei figli del padre */
    list_add(&p->p_sib, &prnt->p_child);
}

// Rimuove e ritorna il primo figlio di p
pcb_t *removeChild(pcb_t *p)
{
    if (emptyChild(p))
    {
        return NULL;
    }
    struct list_head *toRemove = p->p_child.next;
    list_del(toRemove);

    struct pcb_t *toRemovePCB = container_of(toRemove, pcb_t, p_sib);
    toRemovePCB->p_parent = NULL;

    return toRemovePCB;
}

// Rimuove e ritorna il p
pcb_t *outChild(pcb_t *p)
{
    if (p->p_parent == NULL)
        return NULL;
    list_del(&p->p_sib);
    p->p_parent = NULL;
    return p;
}
