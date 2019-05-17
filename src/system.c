#define _DEFAULT_SOURCE
#include "system.h"
#include "file.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

#if defined (__MacOS__) || defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/types.h>
# include <sys/sysctl.h>
# include <sys/param.h>
# include <sys/ucred.h>
# include <sys/mount.h>
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__)
# include <sys/param.h>
# include <sys/sysctl.h>
# include <sys/mount.h>
#endif

int
system_cpu_count(void)
{
   int generic = 1;
   static int cores = 0;

   if (cores != 0)
     return cores;

#if defined (__MacOS__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
   size_t len;
   int mib[2] = { CTL_HW, HW_NCPU };

   len = sizeof(cores);
   if (sysctl(mib, 2, &cores, &len, NULL, 0) != -1)
     return cores;
#elif defined(__linux__)
   char buf[4096];
   FILE *f = fopen("/proc/stat", "r");
   if (!f) return generic;
   int line = 0;
   while (fgets(buf, sizeof(buf), f))
     {
        if (line)
          {
             char *tok = strtok(buf, " ");
             if (!strncmp(tok, "cpu", 3))
               ++cores;
             else break;
          }
        line++;
     }
   fclose(f);

   if (cores)
     return cores;
#endif
   const char *cpu_count = getenv("NUMBER_OF_PROCESSORS");
   if (cpu_count)
     {
        cores = atoi(cpu_count);
        return cores;
     }
   return generic;
}

char *
system_disk_mount_point_get(const char *path)
{
#if defined(__MacOS__) || defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)
   struct statfs *mounts;
   int i, count;

   count = getmntinfo(&mounts, MNT_WAIT);
   for (i = 0; i < count; i++)
     {
        if (!strcmp(path, mounts[i].f_mntfromname))
          {
             return strdup(mounts[i].f_mntonname);
          }
     }
#elif defined(__linux__)
   char buf[4096];
   char *start, *end;
   FILE *f = fopen("/proc/mounts", "r");

   while ((fgets(buf, sizeof(buf), f)) != NULL)
     {
        start = &buf[0];
        end = strchr(start, ' ');
        if (!end) continue;
        *end = 0x0;

        if (!strcmp(path, start))
          {
             start = end + 1;
             if (!start) continue;
             end = strchr(start, ' ');
             if (!end) continue;
             *end = 0x0;
             fclose(f);

             return strdup(start);
          }
     }

   fclose(f);

   return NULL;
#else
#endif
   return NULL;
}

static int
_cmp_cb(void *p1, void *p2)
{
   const char *s1, *s2;

   s1 = p1; s2 = p2;

   return strcmp(s1, s2);
}

list_t *
system_disks_get(void)
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
   struct statfs *mounts;
   int i, count;
   char *drives, *dev, *end;
   char buf[4096];
   size_t len;

   if ((sysctlbyname("kern.disks", NULL, &len, NULL, 0)) == -1)
     return NULL;

   drives = malloc(len + 1);

   if ((sysctlbyname("kern.disks", drives, &len, NULL, 0)) == -1)
     {
        free(drives);
        return NULL;
     }

   list_t *list = list_new();
   dev = drives;
   while (dev)
     {
        end = strchr(dev, ' ');
        if (!end)
          break;

        *end = '\0';

        snprintf(buf, sizeof(buf), "/dev/%s", dev);

        list = list_add(list, strdup(buf));

        dev = end + 1;
     }

   free(drives);

   count = getmntinfo(&mounts, MNT_WAIT);
   for (i = 0; i < count; i++)
     {
        list = list_add(list, strdup(mounts[i].f_mntfromname));
     }

   list = list_sort(list, _cmp_cb);

   return list;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
   static const int mib[] = { CTL_HW, HW_DISKNAMES };
   static const unsigned int miblen = 2;
   struct statfs *mounts;
   char *drives, *dev, *end;
   char buf[4096];
   size_t len;
   int i, count;

   if ((sysctl(mib, miblen, NULL, &len, NULL, 0)) == -1)
     return NULL;

   drives = malloc(len + 1);

   if ((sysctl(mib, miblen, drives, &len, NULL, 0)) == -1)
     {
        free(drives);
	return NULL;
     }

   list_t *list = list_new();

   dev = drives;
   while (dev)
     {
        end = strchr(dev, ':');
	if (!end) break;

        *end = '\0';

	if (dev[0] == ',')
          dev++;

	snprintf(buf, sizeof(buf), "/dev/%s", dev);

        list = list_add(list, strdup(buf));

        end++;
	dev = strchr(end, ',');
	if (!dev) break;
    }

   free(drives);

   count = getmntinfo(&mounts, MNT_WAIT);
   for (i = 0; i < count; i++)
     {
        list = list_add(list, strdup(mounts[i].f_mntfromname));
     }

   list = list_sort(list, _cmp_cb);

   return list;
#elif defined(__MacOS__)
   char *name;
   char buf[4096];
   list_t *devs, *l, *list;

   list  = list_new();

   devs = file_ls("/dev");

   l = devs;
   while (l)
     {
        name = l->data;
        if (!strncmp(name, "disk", 4))
          {
             snprintf(buf, sizeof(buf), "/dev/%s", name);
             list = list_add(list, strdup(buf));
          }
        l = l->next;
     }

   list_free(devs);

   list = list_sort(list, _cmp_cb);

   return list;
#elif defined(__linux__)
   char *name;
   list_t *l, *devs, *list;
   char buf[4096];

   list = list_new();

   devs = file_ls("/dev/disk/by-path");

   l = devs;
   while (l)
     {
        name = l->data;
        snprintf(buf, sizeof(buf), "/dev/disk/by-path/%s", name);
        char *real = realpath(buf, NULL);
        if (real)
          {
             list = list_add(list, real);
          }
        l = l->next;
     }

   list_free(devs);

   devs = file_ls("/dev/mapper");
   l = devs;
   while (l)
     {
        name = l->data;
        snprintf(buf, sizeof(buf), "/dev/mapper/%s", name);
        list = list_add(list, strdup(buf));
        l = l->next;
     }

   list_free(devs);


   list = list_sort(list, _cmp_cb);

   return list;
#else

   return NULL;
#endif
}
