#include "proc.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
   list_t *list, *l;
   proc_t *proc;

   list = proc_info_all_get();

   LIST_FOREACH(list, l, proc)
     {
        printf("pid: %d uid: %d nice: %d pri: %d cpu: %d thr: %u size: %u K rss: %u K cmd: %s state: (%s)\n",
               proc->pid, proc->uid, proc->nice, proc->priority, proc->cpu_id, proc->numthreads, proc->mem_size >> 10,
               proc->mem_rss >> 10, proc->command, proc->state);
     }

   list_free(list);

   proc = proc_info_by_pid(1);
   if (proc)
     {
        printf("pid: %d uid: %d nice: %d pri: %d cpu: %d thr: %d size: %u K rss: %u K cmd: %s state: (%s)\n",
               proc->pid, proc->uid, proc->nice, proc->priority, proc->cpu_id, proc->numthreads, proc->mem_size >> 10,
               proc->mem_rss >> 10, proc->command, proc->state);
        free(proc);
     }

   return 0;
}
