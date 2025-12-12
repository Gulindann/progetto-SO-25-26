#include "./headers/asl.h"
#include "./headers/pcb.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h;
static struct list_head semd_h;

void initASL() {
  INIT_LIST_HEAD(&semdFree_h);
  INIT_LIST_HEAD(&semd_h);
  for (int i = 0; i < MAXPROC; i++) {
    INIT_LIST_HEAD(&semd_table[i].s_link);
    INIT_LIST_HEAD(&semd_table[i].s_procq);
    list_add(&semd_table[i].s_link, &semdFree_h);
  }
}

int insertBlocked(int* semAdd, pcb_t* p) {
    /*struct semd_t* sem = container_of(semAdd, semd_t, s_key);
    if(!emptyProcQ(&sem->s_procq))
    {
        p->p_semAdd = semAdd;
        list_add_tail(&p->p_list, &sem->s_procq);
        return 0;
    }
    */
    struct list_head *temp;
    list_for_each(temp,&semd_h)
    {
        if(container_of(temp, semd_t, s_link)->s_key == semAdd)
        {
          p->p_semAdd = semAdd;
          list_add_tail(&p->p_list, &container_of(temp, semd_t, s_link)->s_procq);
          return 0;
        }
    }

    if(list_empty(&semdFree_h))
    {
        return 1;
    }

    struct semd_t *sem1 = container_of(semdFree_h.next, semd_t, s_link);
    list_del(semdFree_h.next);
    list_add(&sem1->s_link, &semd_h);
    sem1->s_key = semAdd;
    mkEmptyProcQ(&sem1->s_procq);
    list_add_tail(&p->p_list, &sem1->s_procq);
    p->p_semAdd = semAdd;
    return 0;        
}

pcb_t* removeBlocked(int* semAdd) {
  struct list_head *temp;
  struct pcb_t *toRemovePCB;
  if(list_empty(&semd_h))
  {
    return NULL;
  }

  list_for_each(temp, &semd_h)
  {
    struct semd_t *toRemove = container_of(temp, semd_t, s_link);
    if(toRemove->s_key == semAdd)
    {      
      toRemovePCB = container_of(toRemove->s_procq.next, pcb_t, p_list); 
      list_del(toRemove->s_procq.next);
      if(emptyProcQ(&toRemove->s_procq))
      {
        list_del(&toRemove->s_link);
        list_add(&toRemove->s_link, &semdFree_h);
      }
      return toRemovePCB;
    }
  }  
  return NULL;
}

pcb_t* outBlocked(pcb_t* p) {
  struct list_head *tmp;
  struct list_head *tmp1;
  list_for_each(tmp, &semd_h)
  {
      struct semd_t *sbr = container_of(tmp, semd_t, s_link);
      if(p->p_semAdd == sbr->s_key)
      {
        list_for_each(tmp1, &sbr->s_procq)
        {
          if(&p->p_list == tmp1)
          {
            list_del(tmp1);
            if(emptyProcQ(&sbr->s_procq))
            {
              list_del(&sbr->s_link);
              list_add(&sbr->s_link, &semdFree_h);
            }            
            return p;
          }
        }
        return NULL;
        //list_del(&p->p_list);
      }
  }
  return NULL;
}

pcb_t* headBlocked(int* semAdd) {
  struct list_head* temp;
  
  list_for_each(temp, &semd_h)
  {
    struct semd_t *sbr = container_of(temp, semd_t, s_link);
    if(sbr->s_key == semAdd)
    {
      return container_of(sbr->s_procq.next, pcb_t, p_list);
    }
  }
  return NULL;

}
