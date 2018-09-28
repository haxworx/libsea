#ifndef __SYSTEM_H__
#define __SYSTEM_H__
#include <sys/types.h>
#include "list.h"

/**
 * @file
 * @brief System specific operations that aren't usually portable.
 */

/**
 * @brief Useful system operations.
 * @defgroup System
 *
 * @{
 *
 * System operations.
 *
 */

#if defined(__APPLE__) && defined(__MACH__)
#define __MacOS__
#endif


/**
 * Return the number of CPUs available to the operating system.
 *
 * @return The number of available CPUs.
 */
int
system_cpu_count(void);

/**
 * Return a list of available disks/drives on the system as (char *).
 *
 * @return A list of device paths.
 */
list_t *
system_disks_get(void);

/**
 * Return the mount point of a device path if mounted.
 *
 * @param path The device location.
 *
 * @return A newly allocated string containing the mount point if found otherwise NULL if the device is not mounted.
 */
char *
system_disk_mount_point_get(const char *path);

/**
 * @}
 */

#endif

