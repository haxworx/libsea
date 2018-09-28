#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static lock_t lock;

static void *
_thread_end_cb(thread_t *thread, void *data)
{
   printf("\nThread ended\n");

   return NULL;
}

static void *
_thread_cancel_cb(thread_t *thread, void *data)
{
   printf("Thread cancelled!\n");

   return NULL;
}

static void *
_thread_worker(thread_t *thread, void *data)
{
  for (int i = 0; i < 15; i++)
   {
      printf("thread says: %d\n", i);
      sleep(1);
   }

   return NULL;
}

static void *
_thread_worker_feedback(thread_t *thread, void *data)
{
  for (int i = 0; i <= 100; i++)
   {
      thread_feedback(thread, (void *) &i);
      usleep(100000);
   }

   return NULL;
}

static void *
_thread_feedback_cb(thread_t *thread, void *data)
{
   int *i = data;

   lock_take(&lock);

   printf("feedback: %d%% \r", *i);
   fflush(stdout);

   lock_release(&lock);

   return NULL;
}

int
main(void)
{
   thread_t *thread;

   puts("testing: thread_run()");

   thread = thread_run(_thread_worker, _thread_end_cb, _thread_cancel_cb, NULL);

   for (int i = 0; i < 10; i++)
     {
        sleep(1);
        //if (i == 8) thread_cancel(thread);
        printf("Main program!\n");
     }

   puts("waiting");
   thread_wait(thread);

   free(thread);

   puts("testing: thread_feedback_run()");

   lock_init(&lock);

   thread = thread_feedback_run(_thread_worker_feedback, _thread_end_cb, _thread_cancel_cb, _thread_feedback_cb, NULL);

   thread_wait(thread);

   lock_destroy(&lock);

   free(thread);
}
