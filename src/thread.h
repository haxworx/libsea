#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

/**
 * @file
 * @brief These routines for managing thread creation and manipulation.
 */
typedef struct thread_t thread_t;

typedef void *(*thread_run_cb)(thread_t *thread, void *data);
typedef void *(*thread_end_cb)(thread_t *thread, void *data);
typedef void *(*thread_cancel_cb)(thread_t *thread, void *data);
typedef void *(*thread_feedback_cb)(thread_t *thread, void *data);

/**
 * @brief Thread creation and manipulation.
 * @defgroup Thread
 *
 * @{
 *
 * Creation and manipulation of threads.
 *
 *
 */
struct thread_t
{
   pthread_t id;
   void     *data;

   thread_run_cb run_cb;
   thread_cancel_cb cancel_cb;
   thread_feedback_cb feedback_cb;
   thread_end_cb end_cb;
};

/**
 * Run a new thread.
 *
 * @param thread_run The main thread execution function.
 * @param thread_end The function to execute on completion of thread execution.
 * @param thread_cancel The function to execute on cancellation of a thread.
 * @param data User data to pass to the thread and other callbacks.
 *
 * @return A pointer to the thread if successfully created, otherwise NULL.
 */
thread_t *
thread_run(thread_run_cb thread_run, thread_end_cb thread_end, thread_cancel_cb thread_cancel, void *data);

/**
 * Run a new thread with feedback.
 *
 * @param thread_run The main thread execution function.
 * @param thread_end The function to execute on completion of thread execution.
 * @param thread_cancel The function to execute on cancellation of a thread.
 * @param thread_feedback The function to be called when feedback is requested within the executing thread.
 * @param data User data to pass to the thread and other callbacks.
 *
 * @return A pointer to the thread if successfully created, otherwise NULL.
 */
thread_t *
thread_feedback_run(thread_run_cb thread_run, thread_end_cb thread_end, thread_cancel_cb thread_cancel, thread_feedback_cb thread_feedback, void *data);

/**
 * Feedback data from the thread to the feedback callback specified when calling thread_feedback_run().
 *
 * @param thread The thread instance.
 * @param data The data to pass back to the feedback callback.
 *
 * @return A pointer to the thread.
 */
thread_t *
thread_feedback(thread_t *thread, void *data);

/**
 * Cancel a running thread.
 *
 * @param thread The thread to cancel.
 *
 * @return Returns zero on success otherwise non-zero.
 */
int
thread_cancel(thread_t *thread);

/**
 * Wait for a thread to finish executing.
 *
 * @param thread The thread to wait for.
 *
 * @return Returns zero on success otherwise non-zero.
 */
int
thread_wait(thread_t *thread);

typedef pthread_mutex_t lock_t;

/**
 * Initialize a lock.
 *
 * @param lock A pointer to the lock.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
lock_init(lock_t *lock);

/**
 * Take the lock and block other threads.
 *
 * @param lock The lock instance.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
lock_take(lock_t *lock);

/**
 * Release a locked lock.
 *
 * @param lock The lock to release.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
lock_release(lock_t *lock);

/**
 * Destroy a lock after it has been used freeing up memory and resources.
 *
 * @param lock The lock to destroy.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
lock_destroy(lock_t *lock);

typedef pthread_spinlock_t spinlock_t;

/**
 * Initialize a spinlock.
 *
 * @param spinlock A pointer to the spinlock;
 *
 * @return Return zero on success otherwise non-zero.
 */
int
spinlock_init(spinlock_t *spinlock);

/**
 * Destroy a lock after it has been used freeing up memory and resources.
 *
 * @param spinlock The lock to destroy.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
spinlock_destroy(spinlock_t *spinlock);

/**
 * Lock the spinlock or wait continuously checking until locked.
 *
 * @param spinlock The spinlock to lock or test and then lock.
 *
 * @return Return zero on success, otherwise non-zero.
 */
int
spinlock_take(spinlock_t *spinlock);

/*
 * Try to lock the spinlock but if locked return.
 *
 * @param spinlock The spinlock to test and try to lock.
 *
 * @return zero on successful lock otherwise if locked errno is EBUSY and non-zero.
 */
int
spinlock_taketry(spinlock_t *spinlock);

/**
 * Release a locked spinlock.
 *
 * @param spinlock The spinlock to release.
 *
 * @return Return zero on success otherwise non-zero.
 */
int
spinlock_release(spinlock_t *spinlock);


/**
 * @}
 */

#endif
