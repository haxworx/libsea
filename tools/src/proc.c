#if defined(__MACH__) && defined(__APPLE__)
# define __MacOS__
#endif

#if defined(__MacOS__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
# include <sys/types.h>
# include <sys/sysctl.h>
# include <sys/user.h>
# include <sys/proc.h>
#endif

#if defined(__OpenBSD__)
# include <kvm.h>
# include <limits.h>
# include <sys/proc.h>
# include <sys/param.h>
#endif

#if defined(__MacOS__)
# include <libproc.h>
# include <sys/proc_info.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "file.h"
#include "proc.h"
#include "list.h"

#if !defined(PID_MAX)
# define PID_MAX 99999
#endif

static const char *
_process_state_name(char state)
{
   const char *statename = NULL;
#if defined(__linux__)

   switch (state)
     {
        case 'D':
          statename = "DSLEEP";
        break;

        case 'I':
          statename = "IDLE";
        break;

        case 'R':
          statename = "RUN";
        break;

        case 'S':
          statename = "SLEEP";
        break;

        case 'T':
        case 't':
          statename = "STOP";
        break;

        case 'X':
          statename = "DEAD";
        break;

        case 'Z':
          statename = "ZOMB";
        break;
     }
#else
   switch (state)
     {
        case SIDL:
           statename = "IDLE";
        break;

        case SRUN:
           statename = "RUN";
        break;

        case SSLEEP:
          statename = "SLEEP";
        break;

        case SSTOP:
          statename = "STOP";
        break;
#if !defined(__MacOS__)
#if !defined(__OpenBSD__)
        case SWAIT:
          statename = "WAIT";
        break;

        case SLOCK:
           statename = "LOCK";
        break;
#endif
        case SZOMB:
          statename = "ZOMB";
        break;
#endif
#if defined(__OpenBSD__)
        case SDEAD:
          statename = "DEAD";
        break;

        case SONPROC:
          statename = "ONPROC";
        break;
#endif
     }
#endif
   return statename;
}

#if defined(__linux__)

static unsigned long
_parse_line(const char *line)
{
   char *p, *tok;

   p = strchr(line, ':') + 1;
   while (isspace(*p))
     p++;
   tok = strtok(p, " ");

   return atol(tok);
}

static list_t *
_process_list_linux_get(void)
{
   char *name;
   list_t *files, *l, *list = NULL;
   FILE *f;

   char path[PATH_MAX], line[4096], program_name[1024], state;
   int pid, res, utime, stime, cutime, cstime, uid, psr, pri, nice, numthreads;
   unsigned int mem_size, mem_rss;

   int pagesize = getpagesize();

   files = file_ls("/proc");
   LIST_FOREACH(files, l, name)
     {
        pid = atoi(name);
        if (!pid) continue;

        snprintf(path, sizeof(path), "/proc/%d/stat", pid);

        f = fopen(path, "r");
        if (!f) continue;

        if (fgets(line, sizeof(line), f))
          {
             int dummy;
             char *end, *start = strchr(line, '(') + 1;
             end = strchr(line, ')');
             strncpy(program_name, start, end - start);
             program_name[end - start] = '\0';

             res = sscanf (end + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u %d %d %d %d %d %d %d %d %d",
                           &state, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &utime, &stime, &cutime, &cstime,
                           &pri, &nice, &numthreads, &dummy, &dummy, &mem_size, &mem_rss, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
                           &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &psr, &dummy, &dummy, &dummy, &dummy, &dummy);
          }

        fclose(f);

        if (res != 42) continue;

        snprintf(path, sizeof(path), "/proc/%d/status", pid);

        f = fopen(path, "r");
        if (!f) continue;

        while ((fgets(line, sizeof(line), f)) != NULL)
          {
             if (!strncmp(line, "Uid:", 4))
               {
                   uid = _parse_line(line);
                   break;
               }
          }

        fclose(f);

        proc_t *p = calloc(1, sizeof(proc_t));

        p->pid = pid;
        p->uid = uid;
        p->cpu_id = psr;
        snprintf(p->command, sizeof(p->command), "%s", program_name);
        p->state = _process_state_name(state);
        p->cpu_time = utime + stime;
        p->mem_size = mem_size;
        p->mem_rss = mem_rss * pagesize;
        p->nice = nice;
        p->priority = pri;
        p->numthreads = numthreads;

        list = list_add(list, p);
     }

   if (files)
     list_free(files);

   return list;
}

proc_t *
proc_info_by_pid(int pid)
{
   FILE *f;
   char path[PATH_MAX];
   char line[4096];
   char state, program_name[1024];
   int res, dummy, utime, stime, cutime, cstime, uid, psr;
   unsigned int mem_size, mem_rss, pri, nice, numthreads;

   snprintf(path, sizeof(path), "/proc/%d/stat", pid);
   if (!file_exists(path))
     return NULL;

   f = fopen(path, "r");
   if (!f) return NULL;

   if (fgets(line, sizeof(line), f))
     {
        char *end, *start = strchr(line, '(') + 1;
        end = strchr(line, ')');
        strncpy(program_name, start, end - start);
        program_name[end - start] = '\0';

        res = sscanf (end + 2, "%c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u %d %d %d %d %d %d %d %d %d",
                      &state, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &utime, &stime, &cutime, &cstime,
                      &pri, &nice, &numthreads, &dummy, &dummy, &mem_size, &mem_rss, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
                      &dummy, &dummy, &dummy, &dummy, &dummy, &dummy, &psr, &dummy, &dummy, &dummy, &dummy, &dummy);
     }
   fclose(f);

   if (res != 42) return NULL;

   snprintf(path, sizeof(path), "/proc/%d/status", pid);

   f = fopen(path, "r");
   if (!f) return NULL;

   while ((fgets(line, sizeof(line), f)) != NULL)
     {
        if (!strncmp(line, "Uid:", 4))
          {
             uid = _parse_line(line);
             break;
          }
     }
   fclose(f);

   proc_t *p = calloc(1, sizeof(proc_t));
   p->pid = pid;
   p->uid = uid;
   p->cpu_id = psr;
   snprintf(p->command, sizeof(p->command), "%s", program_name);
   p->state = _process_state_name(state);
   p->cpu_time = utime + stime;
   p->mem_size = mem_size;
   p->mem_rss = mem_rss * getpagesize();
   p->priority = pri;
   p->nice = nice;
   p->numthreads = numthreads;

   return p;
}

#endif

#if defined(__OpenBSD__)

proc_t *
proc_info_by_pid(int pid)
{
   struct kinfo_proc *kp;
   kvm_t *kern;
   char errbuf[4096];
   int count, pagesize, pid_count;

   kern = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
   if (!kern) return NULL;

   kp = kvm_getprocs(kern, KERN_PROC_PID, pid, sizeof(*kp), &count);
   if (!kp) return NULL;

   if (count == 0) return NULL;
   pagesize = getpagesize();

   proc_t *p = malloc(sizeof(proc_t));
   p->pid = kp->p_pid;
   p->uid = kp->p_uid;
   p->cpu_id = kp->p_cpuid;
   snprintf(p->command, sizeof(p->command), "%s", kp->p_comm);
   p->state = _process_state_name(kp->p_stat);
   p->cpu_time = kp->p_uticks + kp->p_sticks + kp->p_iticks;
   p->mem_size = (kp->p_vm_tsize * pagesize) + (kp->p_vm_dsize * pagesize) + (kp->p_vm_ssize * pagesize);
   p->mem_rss = kp->p_vm_rssize * pagesize;
   p->priority = kp->p_priority - PZERO;
   p->nice = kp->p_nice - NZERO;
   p->numthreads = -1;

   kp = kvm_getprocs(kern, KERN_PROC_SHOW_THREADS, 0, sizeof(*kp), &pid_count);

   for (int i = 0; i < pid_count; i++)
     {
        if (kp[i].p_pid == p->pid)
          p->numthreads++;
     }

   kvm_close(kern);

   return p;
}

static list_t *
_process_list_openbsd_get(void)
{
   struct kinfo_proc *kp;
   proc_t *p;
   char errbuf[4096];
   kvm_t *kern;
   int pid_count, pagesize;
   list_t *l, *list = list_new();

   kern = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
   if (!kern) return NULL;

   kp = kvm_getprocs(kern, KERN_PROC_ALL, 0, sizeof(*kp), &pid_count);
   if (!kp) return NULL;

   pagesize = getpagesize();

   for (int i = 0; i < pid_count; i++)
     {
        p = malloc(sizeof(proc_t));
        p->pid = kp[i].p_pid;
        p->uid = kp[i].p_uid;
        p->cpu_id = kp[i].p_cpuid;
        snprintf(p->command, sizeof(p->command), "%s", kp[i].p_comm);
        p->state = _process_state_name(kp[i].p_stat);
        p->cpu_time = kp[i].p_uticks + kp[i].p_sticks + kp[i].p_iticks;
        p->mem_size = (kp[i].p_vm_tsize * pagesize) + (kp[i].p_vm_dsize * pagesize) + (kp[i].p_vm_ssize * pagesize);
        p->mem_rss = kp[i].p_vm_rssize * pagesize;
	p->priority = kp[i].p_priority - PZERO;
	p->nice = kp[i].p_nice - NZERO;
        p->numthreads = -1;
        list = list_add(list, p);
     }

   kp = kvm_getprocs(kern, KERN_PROC_SHOW_THREADS, 0, sizeof(*kp), &pid_count);

   LIST_FOREACH(list, l, p)
     {
        for (int i = 0; i < pid_count; i++)
          {
             if (kp[i].p_pid == p->pid)
	       p->numthreads++;
          }
     }

   kvm_close(kern);

   return list;
}

#endif

#if defined(__MacOS__)
static list_t *
_process_list_macos_get(void)
{
   list_t *list = NULL;

   for (int i = 1; i <= PID_MAX; i++)
     {
        struct proc_taskallinfo taskinfo;
        int size = proc_pidinfo(i, PROC_PIDTASKALLINFO, 0, &taskinfo, sizeof(taskinfo));
        if (size != sizeof(taskinfo)) continue;

        proc_t *p = calloc(1, sizeof(proc_t));
        p->pid = i;
        p->uid = taskinfo.pbsd.pbi_uid;
        p->cpu_id = -1;
        snprintf(p->command, sizeof(p->command), "%s", taskinfo.pbsd.pbi_comm);
        p->cpu_time = taskinfo.ptinfo.pti_total_user + taskinfo.ptinfo.pti_total_system;
        p->cpu_time /= 10000000;
        p->state = _process_state_name(taskinfo.pbsd.pbi_status);
        p->mem_size = taskinfo.ptinfo.pti_virtual_size;
        p->mem_rss = taskinfo.ptinfo.pti_resident_size;
        p->priority = taskinfo.ptinfo.pti_priority;
        p->nice = taskinfo.pbsd.pbi_nice;
        p->numthreads = taskinfo.ptinfo.pti_threadnum;

        list = list_add(list, p);
     }

   return list;
}

proc_t *
proc_info_by_pid(int pid)
{
   struct proc_taskallinfo taskinfo;
   struct proc_workqueueinfo workqueue;
   int size;

   size = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &taskinfo, sizeof(taskinfo));
   if (size != sizeof(taskinfo))
     return NULL;

   size = proc_pidinfo(pid, PROC_PIDWORKQUEUEINFO, 0, &workqueue, sizeof(workqueue));

   proc_t *p = calloc(1, sizeof(proc_t));
   p->pid = pid;
   p->uid = taskinfo.pbsd.pbi_uid;
   p->cpu_id = workqueue.pwq_nthreads;
   snprintf(p->command, sizeof(p->command), "%s", taskinfo.pbsd.pbi_comm);
   p->cpu_time = taskinfo.ptinfo.pti_total_user + taskinfo.ptinfo.pti_total_system;
   p->cpu_time /= 10000000;
   p->state = _process_state_name(taskinfo.pbsd.pbi_status);
   p->mem_size = taskinfo.ptinfo.pti_virtual_size;
   p->mem_rss = taskinfo.ptinfo.pti_resident_size;
   p->priority = taskinfo.ptinfo.pti_priority;
   p->nice = taskinfo.pbsd.pbi_nice;
   p->numthreads = taskinfo.ptinfo.pti_threadnum;

   return p;
}

#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
static list_t *
_process_list_freebsd_get(void)
{
   list_t *list;
   struct rusage *usage;
   struct kinfo_proc kp;
   int mib[4];
   size_t len;
   int pagesize = getpagesize();

   list = list_new();

   len = sizeof(int);
   if (sysctlnametomib("kern.proc.pid", mib, &len) == -1)
     return NULL;

   for (int i = 1; i <= PID_MAX; i++)
     {
        mib[3] = i;
        len = sizeof(kp);
        if (sysctl(mib, 4, &kp, &len, NULL, 0) == -1)
          {
             continue;
          }

        proc_t *p = calloc(1, sizeof(proc_t));

        p->pid = kp.ki_pid;
        p->uid = kp.ki_uid;
        snprintf(p->command, sizeof(p->command), "%s", kp.ki_comm);
        p->cpu_id = kp.ki_oncpu;
        if (p->cpu_id == -1)
          p->cpu_id = kp.ki_lastcpu;

        usage = &kp.ki_rusage;

        p->cpu_time = (usage->ru_utime.tv_sec * 1000000) + usage->ru_utime.tv_usec + (usage->ru_stime.tv_sec * 1000000) + usage->ru_stime.tv_usec;
        p->state = _process_state_name(kp.ki_stat);
        p->mem_size = kp.ki_size;
        p->mem_rss = kp.ki_rssize * pagesize;
        p->nice =  kp.ki_nice - NZERO;
        p->priority = kp.ki_pri.pri_level - PZERO;
        p->numthreads = kp.ki_numthreads;

        list = list_add(list, p);
     }

   return list;
}

proc_t *
proc_info_by_pid(int pid)
{
   struct rusage *usage;
   struct kinfo_proc kp;
   int mib[4];
   size_t len;
   int pagesize = getpagesize();

   len = sizeof(int);
   if (sysctlnametomib("kern.proc.pid", mib, &len) == -1)
     return NULL;

   mib[3] = pid;

   len = sizeof(kp);
   if (sysctl(mib, 4, &kp, &len, NULL, 0) == -1)
     return NULL;

   proc_t *p = calloc(1, sizeof(proc_t));
   p->pid = kp.ki_pid;
   p->uid = kp.ki_uid;
   snprintf(p->command, sizeof(p->command), "%s", kp.ki_comm);
   p->cpu_id = kp.ki_oncpu;
   if (p->cpu_id == -1)
     p->cpu_id = kp.ki_lastcpu;

   usage = &kp.ki_rusage;

   p->cpu_time = (usage->ru_utime.tv_sec * 1000000) + usage->ru_utime.tv_usec + (usage->ru_stime.tv_sec * 1000000) + usage->ru_stime.tv_usec;
   p->state = _process_state_name(kp.ki_stat);
   p->mem_size = kp.ki_size;
   p->mem_rss = kp.ki_rssize * pagesize;
   p->nice = kp.ki_nice = NZERO;
   p->priority = kp.ki_pri.pri_level - PZERO;
   p->numthreads = kp.ki_numthreads;

   return p;
}

#endif

list_t *
proc_info_all_get(void)
{
   list_t *processes;

#if defined(__linux__)
   processes = _process_list_linux_get();
#elif defined(__FreeBSD__) || defined(__DragonFly__)
   processes = _process_list_freebsd_get();
#elif defined(__MacOS__)
   processes = _process_list_macos_get();
#elif defined(__OpenBSD__)
   processes = _process_list_openbsd_get();
#else
   processes = NULL;
#endif

   return processes;
}

