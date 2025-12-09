#include "./headers/asl.h"
#include "./headers/pcb.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

void initASL() {
  INIT_LIST_HEAD(&semdFree_h);
  for (int i = 0; i < MAXPROC; i++) {
    //removeBlocked(&semd_table[i]);
  }
}

int insertBlocked(int* semAdd, pcb_t* p) {
    struct semd_t* sem = container_of(semAdd, semd_t, s_key);
    if(!emptyProcQ(&sem->s_procq))
    {
        p->p_semAdd = semAdd;
        list_add_tail(&p->p_list, &sem->s_procq);
        return 0;
    }
    
    if(list_empty(&semdFree_h))
    {
        return 1;
    }

    sem = container_of(semdFree_h.next, semd_t, s_link);
    list_del(semdFree_h.next);
    list_add(&sem->s_link, &semd_h);
    sem->s_key = semAdd;
    mkEmptyProcQ(&sem->s_procq);
    p->p_semAdd = semAdd;
    return 0;        
}

pcb_t* removeBlocked(int* semAdd) {

}

pcb_t* outBlocked(pcb_t* p) {
}

pcb_t* headBlocked(int* semAdd) {
}
