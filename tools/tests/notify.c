#include "notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROGRAM_NAME "notify"

static void
_change_cb(const char *path, notify_event_t type, void *data)
{
   const char *action = data;

   printf("%s %s\n", action, path);
}

void
usage(void)
{
   printf("%s <path>\n", PROGRAM_NAME);
   exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
   const char *path;

   if (argc != 2)
     usage();

   path = argv[1];

   notify_t *notify = notify_new();

   if (!notify_path_set(notify, path))
     exit(1 << 0);

   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_FILE_ADD, _change_cb, "file add");
   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_FILE_DEL, _change_cb, "file del");
   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_FILE_MOD, _change_cb, "file mod");
   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_DIR_ADD, _change_cb, "dir add");
   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_DIR_DEL, _change_cb, "dir del");
   notify_event_callback_set(notify, NOTIFY_EVENT_CALLBACK_DIR_MOD, _change_cb, "dir mod");

   puts("notify: initializing!");

   if (notify_background_run(notify))
     exit(1 << 1);

   printf("notify: actively monitoring : %s .\n", path);

   for (int i = 0; i < 20; i++)
     {
        puts("doing something else!");
        sleep(1);
     }

   puts("notify: calling notify_stop_wait() from main thread.");

   notify_stop_wait(notify);

   notify_free(notify);

   puts("notify: done.");

   return EXIT_SUCCESS;
}

