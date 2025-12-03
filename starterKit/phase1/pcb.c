#include "./headers/pcb.h"

static struct list_head pcbFree_h;
static pcb_t pcbFree_table[MAXPROC];
static int next_pid = 1;

void initPcbs() {
    
    INIT_LIST_HEAD(&pcbFree_h);
    for(int i = 0;i<MAXPROC;i++)
    {
        freePcb(&pcbFree_table[i]);
    }
    
}

void freePcb(pcb_t* p) {
    list_add(&p->p_list,&pcbFree_h);
}

pcb_t* allocPcb() {
    if(list_empty(&pcbFree_h)==1)
    {
                addokbuf("guarda come panico1 \n");
        return NULL;
    }
    else
    {
                addokbuf("guarda come panico2 \n");
        pcb_t* emptyPcb = container_of(pcbFree_h.next, pcb_t, p_list);
        addokbuf("guarda come panico3 \n");
        emptyPcb->p_list.next = NULL;
        emptyPcb->p_child.next = NULL;
        emptyPcb->p_sib.next = NULL;
        emptyPcb->p_parent = NULL;
        emptyPcb->p_semAdd = NULL;
        emptyPcb->p_supportStruct = NULL;
        emptyPcb->p_prio = 0;
                addokbuf("guarda come panico4 \n");
        list_del(&pcbFree_h);
                addokbuf("guarda come panico5 \n");
        return emptyPcb;
    }
}

void mkEmptyProcQ(struct list_head* head) {
}

int emptyProcQ(struct list_head* head) {
}

void insertProcQ(struct list_head* head, pcb_t* p) {
}

pcb_t* headProcQ(struct list_head* head) {
}

pcb_t* removeProcQ(struct list_head* head) {
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
}

int emptyChild(pcb_t* p) {
}

void insertChild(pcb_t* prnt, pcb_t* p) {
}

pcb_t* removeChild(pcb_t* p) {
}

pcb_t* outChild(pcb_t* p) {
}
