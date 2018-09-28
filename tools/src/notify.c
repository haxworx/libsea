#define _DEFAULT_SOURCE
#include <unistd.h>
#include "notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#if defined(__MACH__) && defined(__APPLE__)
# define __MacOS__
#endif

#if defined(__MacOS__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <sys/types.h>
# include <sys/event.h>
# include <sys/time.h>
# include <fcntl.h>
# define KEVENT_NUM_EVENTS 5
#endif

#if defined(__linux__)
# include <sys/inotify.h>
# include <sys/time.h>
# include <stddef.h>
#endif

static void
_notify_file_list_free(list_t *next_list)
{
   list_t *next, *node = next_list;
   file_info_t *file;

   while (node)
     {
        next = node->next;
        file = node->data;
        free(file->path);
        free(file);
        file = NULL;
        free(node);
        node = NULL;
        node = next;
     }
}

static int
_path_scan_cb(const char *path, stat_t *st, void *data)
{
   file_info_t *entry;
   list_t *l = data;

   entry = malloc(sizeof(file_info_t));
   entry->path = strdup(path);
   entry->st = *st;

   l = list_add(l, entry);

   return 0;
}

static void
_notify_engine_fallback(notify_t *notify)
{
   list_t *next_list;
   list_t *l, *l2;
   file_info_t *file, *file2;
   stat_t *tmp;

   file = malloc(sizeof(file_info_t));
   file->path = strdup(notify->path);
   tmp = file_stat(notify->path);
   file->st = *tmp;
   free(tmp);

   next_list = list_new();
   next_list = list_add(next_list, file);

   file_path_walk(notify->path, _path_scan_cb, next_list);

   l = notify->prev_list;
   while (l)
     {
        file = l->data;
        bool exists = false;

        l2 = next_list;
        while (l2)
          {
             file2 = l2->data;
             if (file->st.inode != file2->st.inode)
               {
                   l2 = list_next(l2);
                   continue;
               }

             if (!strcmp(file->path, file2->path))
               {
                  exists = true;
               }

             if (file->st.mtime != file2->st.mtime)
               {
                  if (S_ISDIR(file->st.mode))
                    {
                       if (notify->dir_modified_cb)
                         notify->dir_modified_cb(file->path, NOTIFY_EVENT_CALLBACK_DIR_MOD, notify->dir_modified_data);
                    }
                  else
                    {
                       if (notify->file_modified_cb)
                         notify->file_modified_cb(file->path, NOTIFY_EVENT_CALLBACK_FILE_MOD, notify->file_modified_data);
                    }
               }
             l2 = list_next(l2);
          }

        if (!exists)
          {
             if (S_ISDIR(file->st.mode))
               {
                  if (notify->dir_deleted_cb)
                    notify->dir_deleted_cb(file->path, NOTIFY_EVENT_CALLBACK_DIR_DEL, notify->dir_deleted_data);
               }
             else
               {
                  if (notify->file_deleted_cb)
                    notify->file_deleted_cb(file->path, NOTIFY_EVENT_CALLBACK_FILE_DEL, notify->file_deleted_data);
               }
          }

        l = list_next(l);
     }

   l = next_list;
   while (l && notify->prev_list)
     {
        file = l->data;
        bool exists = false;

        l2 = notify->prev_list;
        while (l2)
          {
             file2 = l2->data;
             if ((file->st.inode == file2->st.inode) &&
                 (!strcmp(file->path, file2->path)))
               {
                  exists = true;
                  break;
               }

             l2 = list_next(l2);
          }

        if (!exists)
          {
             if (S_ISDIR(file->st.mode))
               {
                  if (notify->dir_added_cb)
                    notify->dir_added_cb(file->path, NOTIFY_EVENT_CALLBACK_DIR_ADD, notify->dir_added_data);
               }
             else
               {
                  if (notify->file_added_cb)
                    notify->file_added_cb(file->path, NOTIFY_EVENT_CALLBACK_FILE_ADD, notify->file_added_data);
               }
          }
        l = list_next(l);
     }

   _notify_file_list_free(notify->prev_list);

   notify->prev_list = next_list;
}

#if defined(__linux__)

static void
_notify_path_add(notify_t *notify, const char *path)
{
   void *tmp;

   for (int i = 0; i < notify->dirs_count; i++)
     {
        notify_watch_t *dir = notify->dirs[i];
        if (dir->wd == -1)
          {
             dir->wd = inotify_add_watch(notify->fd, path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
             dir->path = strdup(path);
             return;
          }
     }

   tmp = realloc(notify->dirs, (1 + notify->dirs_count) * sizeof(notify_watch_t *));
   if (tmp)
     notify->dirs = tmp;

   notify->dirs[notify->dirs_count] = malloc(sizeof(notify_watch_t));
   notify->dirs[notify->dirs_count]->path = strdup(path);
   notify->dirs[notify->dirs_count]->wd = inotify_add_watch(notify->fd, path, IN_CREATE | IN_DELETE | IN_MODIFY);
   notify->dirs_count++;
}

static int
_list_cmp(void *p1, void *p2)
{
   const char *s1 = p1, *s2 = p2;

   return strcmp(s1, s2);
}

static int
_notify_add_walk_cb(const char *path, stat_t *st, void *data)
{
   list_t *files;

   if (!S_ISDIR(st->mode))
     return 0;

   files = data;

   files = list_add(files, strdup(path));

   return 0;
}

static void
_notify_path_recursive_add(notify_t *notify, const char *path)
{
   list_t *l, *files = list_new();

   files = list_add(files, strdup(path));
   file_path_walk(path, _notify_add_walk_cb, files);
   files = list_sort(files, _list_cmp);

   l = files;
   while (l)
     {
        char *path = l->data;
        _notify_path_add(notify, path);

        if (notify->dir_added_cb)
          notify->dir_added_cb(path, NOTIFY_EVENT_CALLBACK_DIR_ADD, notify->dir_added_data);

        l = l->next;
     }

   list_free(files);
}

static void
_notify_path_remove(notify_t *notify, const char *path)
{
   notify_watch_t *dir;
   int i;

   for (i = 0; i < notify->dirs_count; i++)
     {
        dir = notify->dirs[i];
        if (dir->path && !strcmp(path, dir->path))
          {
             inotify_rm_watch(notify->fd, dir->wd);
             dir->wd = -1;
             free(dir->path);
             dir->path = NULL;
             break;
          }
     }
}

static void
_notify_event_handler(notify_t *notify, const char *path, struct inotify_event *event)
{
   char fullpath[4096];

   snprintf(fullpath, sizeof(fullpath), "%s/%s", path, event->name);

   if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO)
     {
        if (event->mask & IN_ISDIR)
          {
             if (notify->dir_added_cb)
               {
                  _notify_path_recursive_add(notify, fullpath);
               }
          }
        else
          {
             if (notify->file_added_cb)
               notify->file_added_cb(fullpath, NOTIFY_EVENT_CALLBACK_FILE_ADD, notify->file_added_data);
          }
     }
   else if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM)
     {
        if (event->mask & IN_ISDIR)
          {
             if (notify->dir_deleted_cb)
               {
                  notify->dir_deleted_cb(fullpath, NOTIFY_EVENT_CALLBACK_DIR_DEL, notify->dir_deleted_data);
                  _notify_path_remove(notify, fullpath);
               }
          }
        else
          {
             if (notify->file_deleted_cb)
               notify->file_deleted_cb(fullpath, NOTIFY_EVENT_CALLBACK_FILE_DEL, notify->file_deleted_data);
          }
     }
   else if (event->mask & IN_MODIFY)
     {
         if (event->mask & IN_ISDIR)
           {
              if (notify->dir_modified_cb)
                notify->dir_modified_cb(fullpath, NOTIFY_EVENT_CALLBACK_DIR_MOD, notify->dir_modified_data);
           }
         else
           {
              if (notify->file_modified_cb)
                notify->file_modified_cb(fullpath, NOTIFY_EVENT_CALLBACK_FILE_MOD, notify->file_modified_data);
           }
     }
}

static char *
_notify_path_by_wd(notify_t *notify, int wd)
{
   for (int i = 0; i < notify->dirs_count; i++)
     {
        if (notify->dirs[i]->wd == wd)
          return notify->dirs[i]->path;
     }

   return NULL;
}

static int
_notify_watch_walk_cb(const char *path, stat_t *st, void *data)
{
   notify_t *notify;

   if (!S_ISDIR(st->mode))
     return 0;

   notify = data;

   _notify_path_add(notify, path);

   return 0;
}

#endif

static void
_notify_watch(notify_t *notify)
{
#if defined(__linux__)
   char event_buf[16384] __attribute__((aligned(__alignof__(struct inotify_event))));
   int fd, i, res;
   fd_set _fds, fds;
   struct inotify_event *event = NULL;
   struct timeval tm;

   FD_ZERO(&_fds);

   fd = inotify_init1(IN_NONBLOCK);
   if (fd == -1)
     goto fallback;

   FD_SET(fd, &_fds);

   notify->fd = fd;

   _notify_path_add(notify, notify->path);

   file_path_walk(notify->path, _notify_watch_walk_cb, notify);

   for (i = 0; i < notify->dirs_count; i++)
     {
        notify_watch_t *dirs = notify->dirs[i];
        dirs->wd = inotify_add_watch(fd, dirs->path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
     }

   notify->ready = true;

   while (notify->enabled)
     {
        fds = _fds;
        tm.tv_sec = 0;
        tm.tv_usec = 100000;

        res = select(fd + 1, &fds, NULL, NULL, &tm);
        if (res == -1)
          exit(1 << 1);

        int index = 0;
        if (FD_ISSET(fd, &fds))
          {
             int length = read(fd, event_buf, sizeof(event_buf));
             if (!length) continue;
             while (index < length)
               {
                  event = (struct inotify_event *) &event_buf[index];
                  if (event->len)
                    {
                       const char *path = _notify_path_by_wd(notify, event->wd);
                       if (path)
                         {
                            _notify_event_handler(notify, path, event);
                         }
                    }
                  index += offsetof (struct inotify_event, name) + event->len;
               }
          }
     }

   for (i = 0; i < notify->dirs_count; i++)
     {
        notify_watch_t *dir = notify->dirs[i];
        if (dir->wd != -1)
          inotify_rm_watch(fd, dir->wd);

        if (dir->path)
          free(dir->path);
        free(dir);
     }

   free(notify->dirs);

   close(fd);
#elif defined(__MacOS__) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
   struct kevent e;
   int fd, kqueue_fd;
   struct kevent events[KEVENT_NUM_EVENTS];
   const struct timespec timeout = { 0, 0 };

   kqueue_fd = kqueue();
   if (kqueue_fd == -1)
     goto fallback;

   fd = open(notify->path, O_RDONLY);
   if (fd == -1) exit(1 << 1);

   EV_SET(&e, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_DELETE | NOTE_WRITE | NOTE_ATTRIB, 0, NULL);
   int res = kevent(kqueue_fd, &e, 1, 0, 0, 0);
   if (res) exit(1 << 2);

   _notify_engine_fallback(notify);

   notify->ready = true;

   while (notify->enabled)
     {
         res = kevent(kqueue_fd, 0, 0, events, KEVENT_NUM_EVENTS, &timeout);

         for (int i = 0; i < res; i++)
           {
             if (events[i].fflags & NOTE_WRITE || events[i].fflags & NOTE_ATTRIB
                 || events[i].fflags & NOTE_DELETE)
               {
                  _notify_engine_fallback(notify);
               }
           }
     }

   close(fd);
#endif
fallback:

   notify->ready = true;
   while (notify->enabled)
     {
        _notify_engine_fallback(notify);
        sleep(2);
     }

   _notify_file_list_free(notify->prev_list);
}

void
notify_stop_wait(notify_t *notify)
{
   void *ret = NULL;

   notify->enabled = false;

   pthread_join(notify->thread, ret);
}

static void *
_notify_watch_thread_cb(void *arg)
{
   notify_t *notify = arg;

   _notify_watch(notify);

   return (void *)0;
}

int
notify_background_run(notify_t *notify)
{
   int error;

   if (!file_exists(notify->path))
     return 1;

   error = pthread_create(&notify->thread, NULL, _notify_watch_thread_cb, notify);
   if (!error)
     notify->enabled = true;

   while (!notify->ready)
     usleep(10000);

   return error;
}

void
notify_free(notify_t *notify)
{
   free(notify->path);
   free(notify);
}

notify_t *
notify_new(void)
{
   notify_t *notify;

   notify = calloc(1, sizeof(notify_t));
   notify->prev_list = list_new();

   return notify;
}

int
notify_path_set(notify_t *notify, const char *directory)
{
   if (!file_is_directory(directory))
     return 0;

   notify->path = strdup(directory);

   return 1;
}

void
notify_event_callback_set(notify_t *notify, notify_event_t type, notify_callback_fn func, void *data)
{
   switch (type)
     {
      case NOTIFY_EVENT_CALLBACK_FILE_ADD:
        notify->file_added_cb = func;
        notify->file_added_data = data;
        break;

      case NOTIFY_EVENT_CALLBACK_FILE_MOD:
        notify->file_modified_cb = func;
        notify->file_modified_data = data;
        break;

      case NOTIFY_EVENT_CALLBACK_FILE_DEL:
        notify->file_deleted_cb = func;
        notify->file_deleted_data = data;
        break;

      case NOTIFY_EVENT_CALLBACK_DIR_ADD:
        notify->dir_added_cb = func;
        notify->dir_added_data = data;
        break;

      case NOTIFY_EVENT_CALLBACK_DIR_MOD:
        notify->dir_modified_cb = func;
        notify->dir_modified_data = data;
        break;

      case NOTIFY_EVENT_CALLBACK_DIR_DEL:
        notify->dir_deleted_cb = func;
        notify->dir_deleted_data = data;
        break;
     }
}

