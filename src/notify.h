#ifndef __NOTIFY_H__
#define __NOTIFY_H__

/**
 * @file
 * @brief Monitor filesystem for changes.
 */

#include "list.h"
#include "file.h"
#include <pthread.h>
#include <unistd.h>

/**
 * @brief Monitor a filesystem and act on changes.
 * @defgroup Notify
 *
 * @{
 *
 * Monitor a filesystem for changes.
 */
typedef enum
{
   NOTIFY_EVENT_CALLBACK_FILE_ADD,
   NOTIFY_EVENT_CALLBACK_FILE_DEL,
   NOTIFY_EVENT_CALLBACK_FILE_MOD,
   NOTIFY_EVENT_CALLBACK_DIR_ADD,
   NOTIFY_EVENT_CALLBACK_DIR_DEL,
   NOTIFY_EVENT_CALLBACK_DIR_MOD,
} notify_event_t;

typedef void (*notify_callback_fn)(const char *path, notify_event_t type, void *data);


typedef struct notify_watch_t
{
   int wd;
   char *path;
} notify_watch_t;

typedef struct notify_t
{
   char                *path;
   list_t              *prev_list;
   int                  enabled;
   int                  ready;

   pthread_t            thread;
   notify_callback_fn file_added_cb;
   notify_callback_fn file_deleted_cb;
   notify_callback_fn file_modified_cb;

   notify_callback_fn dir_added_cb;
   notify_callback_fn dir_deleted_cb;
   notify_callback_fn dir_modified_cb;

   int                 fd;
   notify_watch_t     **dirs;
   int                 dirs_count;

   void                *file_added_data;
   void                *file_deleted_data;
   void                *file_modified_data;

   void                *dir_added_data;
   void                *dir_deleted_data;
   void                *dir_modified_data;
} notify_t;

typedef struct file_info_t
{
   stat_t st;
   char  *path;
} file_info_t;

/**
 * Create a new notify instance.
 *
 * @return A pointer to the newly created object.
 */
notify_t *
notify_new(void);

/**
 * Free all resources associated with a notify_t object.
 *
 * @param notify The notify object to free.
 */
void
notify_free(notify_t *notify);

/**
 * Set the path to monitor for changes (recursive).
 *
 * @param notify The notify object.
 * @param directory The directory to recursively monitor.
 *
 * @return Return non-zero on success or zero on failure.
 */
int
notify_path_set(notify_t *notify, const char *directory);

/**
 * Set a callback to be triggered on specified event.
 *
 * @param notify The notify object instance.
 * @param type The event type to set the callback for.
 * @param func The function to be called when event is triggered.
 * @param data User data to be passed to the callback associated with given event.
 */
void
notify_event_callback_set(notify_t *notify, notify_event_t type, notify_callback_fn func, void *data);

/**
 * Begin the notify object's main loop.
 *
 * @param notify The notify_t instance.
 *
 * @return Zero on success and non-zero on failure.
 */
int
notify_background_run(notify_t *notify);

/**
 * Ask notify to stop and wait for it.
 *
 * @param notify The notify instance to stop and wait for.
 */
void
notify_stop_wait(notify_t *notify);

/**
 * @}
 */

#endif
