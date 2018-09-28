#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

static void *
_thread_run_cb(void *data)
{
   void *ret = NULL;
   thread_t *thread = data;

   if (thread->run_cb)
     thread->run_cb(thread, thread->data);

   if (thread->end_cb)
     thread->end_cb(thread, thread->data);

   pthread_join(thread->id, ret);

   return (void *) 0;
}

int
thread_cancel(thread_t *thread)
{
   int res;
   void *ret = NULL;

   res = pthread_cancel(thread->id);

   if (thread->cancel_cb)
     thread->cancel_cb(thread, thread->data);

   pthread_join(thread->id, ret);

   return res;
}

int
thread_wait(thread_t *thread)
{
   void *ret = NULL;

   pthread_join(thread->id, ret);

   return 0;
}

thread_t *
thread_feedback(thread_t *thread, void *data)
{
   if (thread->feedback_cb)
     thread->feedback_cb(thread, data);
   return thread;
}

thread_t *
thread_run(thread_run_cb thread_run, thread_end_cb thread_end , thread_cancel_cb thread_cancel, void *data)
{
   int error;
   thread_t *thread = calloc(1, sizeof(thread_t));

   thread->data = data;
   thread->run_cb = thread_run;
   thread->end_cb = thread_end;
   thread->cancel_cb = thread_cancel;

   error = pthread_create(&thread->id, NULL, _thread_run_cb,  thread);
   if (error)
     {
        free(thread);
        return NULL;
     }

   return thread;
}

thread_t *
thread_feedback_run(thread_run_cb thread_run, thread_end_cb thread_end, thread_cancel_cb thread_cancel, thread_feedback_cb thread_feedback, void *data)
{
   int error;
   thread_t *thread = calloc(1, sizeof(thread_t));

   thread->data = data;
   thread->run_cb = thread_run;
   thread->end_cb = thread_end;
   thread->cancel_cb = thread_cancel;
   thread->feedback_cb = thread_feedback;

   error = pthread_create(&thread->id, NULL, _thread_run_cb, thread);
   if (error)
     {
        free(thread);
        return NULL;
     }

   return thread;
}

int
lock_init(lock_t *lock)
{
   return pthread_mutex_init(lock, NULL);
}

int
lock_take(lock_t *lock)
{
   return pthread_mutex_lock(lock);
}

int
lock_release(lock_t *lock)
{
   return pthread_mutex_unlock(lock);
}

int
lock_destroy(lock_t *lock)
{
   return pthread_mutex_destroy(lock);
}

int
spinlock_init(spinlock_t *spinlock)
{
   return pthread_spin_init(spinlock, PTHREAD_PROCESS_PRIVATE);
}

int
spinlock_destroy(spinlock_t *spinlock)
{
   return pthread_spin_destroy(spinlock);
}

int
spinlock_take(spinlock_t *spinlock)
{
   return pthread_spin_lock(spinlock);
}

int spinlock_taketry(spinlock_t *spinlock)
{
   return pthread_spin_trylock(spinlock);
}

int
spinlock_release(spinlock_t *spinlock)
{
   return pthread_spin_unlock(spinlock);
}

