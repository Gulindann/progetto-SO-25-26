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

    emptyPcb->p_list.next = NULL;
    emptyPcb->p_list.prev = NULL;
    emptyPcb->p_parent = NULL;
    emptyPcb->p_child.next = NULL;
    emptyPcb->p_child.prev = NULL;
    emptyPcb->p_sib.next = NULL;
    emptyPcb->p_sib.prev = NULL;
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

void mkEmptyProcQ(struct list_head *head) {
  INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head) {
  return list_empty(head);
}

void insertProcQ(struct list_head *head, pcb_t *p) {
  if(list_empty(head))
  {
    list_add(&p->p_list, head);
    return;
  }
  struct list_head* curr = head->next;
  while ((container_of(curr, pcb_t, p_list)->p_prio >= p->p_prio) && (curr != head))
  {
    curr = curr->next;   
  }
  list_add(&p->p_list, curr->prev);
}

pcb_t *headProcQ(struct list_head *head) {
  if(list_empty(head))
  {
    return NULL;
  }
  return container_of(head->next, pcb_t, p_list);
}

pcb_t *removeProcQ(struct list_head *head) {
  if(list_empty(head))
  {
    return NULL;
  }
  pcb_t* toRemove = container_of(head->next, pcb_t, p_list);
  list_del(&toRemove->p_list);
  return toRemove;
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
  if(list_empty(head))
  {
    return NULL;
  }
  struct list_head* curr= head->next;
  while(curr != head)
  {
    if(container_of(curr, pcb_t, p_list) == p)
    {
      list_del(&p->p_list);
      return p;
    }
    curr = curr->next;
  }
  return NULL;
}

int emptyChild(pcb_t *p) {

}

void insertChild(pcb_t *prnt, pcb_t *p) {}

pcb_t *removeChild(pcb_t *p) {}

pcb_t *outChild(pcb_t *p) {}
