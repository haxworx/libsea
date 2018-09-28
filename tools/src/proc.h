#ifndef __PROC_H__
#define __PROC_H__

/**
 * @file
 * @brief Routines for querying processes.
 */

/**
 * @brief Querying Processes
 * @defgroup Proc
 *
 * @{
 *
 * Query processes.
 *
 */

#include "list.h"
#include <stdint.h>
#include <unistd.h>

#define CMD_NAME_MAX 2048

typedef struct proc_t
{
   pid_t       pid;
   uid_t       uid;
   char        command[CMD_NAME_MAX];
   int         cpu_id;
   long        cpu_time;
   int8_t      priority;
   int8_t      nice;
   int32_t     numthreads;
   const char *state;
   uint64_t    mem_size;
   uint64_t    mem_rss;
} proc_t;

/**
 * Query a full list of running processes and return a list.
 *
 * @return A list of proc_t members for all processes.
 */
list_t *
proc_info_all_get(void);

/**
 * Query a process for its current state.
 *
 * @param pid The process ID to query.
 *
 * @return A proc_t pointer containing the process information.
 */
proc_t *
proc_info_by_pid(int pid);

/**
 * @}
 */

#endif
