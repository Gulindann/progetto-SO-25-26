#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

void initPcbs() {

  INIT_LIST_HEAD(&pcbFree_h);
  for (int i = 0; i < MAXPROC; i++) {
    freePcb(&pcbFree_table[i]);
  }
}

void freePcb(pcb_t *p) { list_add(&p->p_list, &pcbFree_h); }

pcb_t *allocPcb() {

  if (list_empty(&pcbFree_h)) {
    return NULL;

  } else {

    struct list_head *nodo = pcbFree_h.next;
    list_del(nodo);

    pcb_t *emptyPcb = container_of(nodo, pcb_t, p_list);

    // Inizializzo le liste per il prossimo PCB
    INIT_LIST_HEAD(&emptyPcb->p_list);
    INIT_LIST_HEAD(&emptyPcb->p_child);
    INIT_LIST_HEAD(&emptyPcb->p_sib);

    emptyPcb->p_parent = NULL;
    emptyPcb->p_semAdd = NULL;
    emptyPcb->p_supportStruct = NULL;
    emptyPcb->p_time = 0;
    emptyPcb->p_s.cause = 0;
    emptyPcb->p_s.entry_hi = 0;
    emptyPcb->p_s.mie = 0;
    emptyPcb->p_s.pc_epc = 0;
    emptyPcb->p_s.status = 0;

    for (int i = 0; i < 32; i++) {
      emptyPcb->p_s.gpr[i] = 0;
    }

    emptyPcb->p_prio = 0;

    return emptyPcb;
  }
}

void mkEmptyProcQ(struct list_head *head) { INIT_LIST_HEAD(head); }

int emptyProcQ(struct list_head *head) { return list_empty(head); }

void insertProcQ(struct list_head *head, pcb_t *p) {
  if (list_empty(head)) {
    list_add(&p->p_list, head);
    return;
  }
  struct list_head *curr = head->next;
  while ((container_of(curr, pcb_t, p_list)->p_prio >= p->p_prio) &&
         (curr != head)) {
    curr = curr->next;
  }
  list_add(&p->p_list, curr->prev);
}

pcb_t *headProcQ(struct list_head *head) {
  if (list_empty(head)) {
    return NULL;
  }
  return container_of(head->next, pcb_t, p_list);
}

pcb_t *removeProcQ(struct list_head *head) {
  if (list_empty(head)) {
    return NULL;
  }
  pcb_t *toRemove = container_of(head->next, pcb_t, p_list);
  list_del(&toRemove->p_list);
  return toRemove;
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
  if (list_empty(head)) {
    return NULL;
  }
  struct list_head *curr = head->next;
  while (curr != head) {
    if (container_of(curr, pcb_t, p_list) == p) {
      list_del(&p->p_list);
      return p;
    }
    curr = curr->next;
  }
  return NULL;
}

int emptyChild(pcb_t *p) {
  if (list_empty(&p->p_child)) {
    return 1;
  }
  return 0;
}

/*void insertChild(pcb_t *prnt, pcb_t *p) {
  if (emptyChild(prnt)) {
    prnt->p_child.next = &p->p_child;
    p->p_parent = prnt;
    return;
  } else {
    // p->p_sib.next = &container_of(prnt->p_child.next, pcb_t, p_child)->p_sib;
    // opzione
    p->p_sib.next = &prnt->p_child.next; // qui rendo D un fratello di A
    container_of(prnt->p_child.next, pcb_t, p_child)->p_sib.prev =
        &p->p_sib;                    // qui dico che A ha come fratello D
    p->p_parent = prnt;               // qui rendo P padre di D
    prnt->p_child.next = &p->p_child; // qui dico che P ha come figlio D
  }
}*/

void insertChild(pcb_t *prnt, pcb_t *p) {
  p->p_parent = prnt;
  list_add(&p->p_sib, &prnt->p_child); // Primo fratello di p diventa figlio di
                                       // prnt (se non ha fratelli allora p)
}

/*pcb_t *removeChild(pcb_t *p) {
  if (emptyChild(p)) {
    return NULL;
  }
  pcb_t *toRemove = container_of(p->p_child.next, pcb_t, p_child);
  p->p_child.next = toRemove->p_sib.next;
  toRemove->p_sib.next->prev = NULL;
  toRemove->p_sib.next = NULL;
  toRemove->p_parent = NULL;
  return toRemove;
}*/

pcb_t *removeChild(pcb_t *p) {
  if (list_empty(&p->p_child)) {

    return NULL;
  }
  struct list_head *toRemove = p->p_child.next;
  list_del(toRemove);

  struct pcb_t *toRemovePCB = container_of(toRemove, pcb_t, p_sib);
  toRemovePCB->p_parent = NULL;

  return toRemovePCB;
}

/*pcb_t *outChild(pcb_t *p) {
  if (p->p_parent == NULL) {
    return NULL;
  }
  if (p->p_sib.prev == NULL) {
    return removeChild(p->p_parent);
  }
  p->p_parent = NULL;
  if (p->p_sib.next == NULL) {
    p->p_sib.prev->next = NULL;
  } else {
    list_del(&p->p_sib);
  }
  return p;
}*/

pcb_t *outChild(pcb_t *p) {
  if (p->p_parent == NULL)
    return NULL;
  list_del(&p->p_sib);
  p->p_parent = NULL;
  return p;
}
