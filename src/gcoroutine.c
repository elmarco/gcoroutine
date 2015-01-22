/*
 * Copyright (C) 2006  Anthony Liguori <anthony@codemonkey.ws>
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2011-2013 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "gcoroutineprivate.h"

/**
 * SECTION:gcoroutine
 * @title: Coroutines
 * @short_description: portable support for coroutines
 * @see_also: #GThread
 *
 * A coroutine is a light concurrent context of execution within the
 * same process address space. The process is responsible for managing
 * which context is running by co-operative multitasking (in
 * comparison, a system thread relies on pre-emptive scheduling)
 *
 * A coroutine has its own stack and is therefore able to preserve
 * state across blocking operations, which traditionally require
 * callback functions and manual marshalling of parameters.
 *
 * On most supported operating systems, switching between coroutines
 * can be faster than switching between threads. When native
 * coroutines aren't available, GLib provides a thread implementation
 * for compatibility fallback support.
 */


/**
 * GCoroutineFunc:
 * @data: data passed to the coroutine
 *
 * Specifies the type of the @func functions passed to g_coroutine_new().
 *
 * Returns: the return value of the coroutine
 * Since: 2.40
 */

/**
 * G_COROUTINE_FUNC:
 *
 * Mark a function that executes in coroutine context.
 *
 * Functions that execute in coroutine context cannot be called directly from
 * normal functions.  In the future it would be nice to enable compiler or
 * static checker support for catching such errors.  This annotation might make
 * it possible and in the meantime it serves as documentation.
 *
 * For example:
 *
 * |[
 *   static void foo(void) G_COROUTINE_FUNC
 *   {
 *       ....
 *   }
 * ]|
 *
 * Since: 2.40
 */


/**
 * Enter each coroutine that was previously marked for restart by
 * g_co_queue_next() or g_co_queue_restart_all().  This function is
 * invoked when the current coroutine yields or terminates.
 */
static void
coroutine_resume_queue (GCoroutine *co) G_COROUTINE_FUNC
{
  /* copy & clear the current resume_queue, then resume */
  GQueue resume_queue = co->resume_queue;

  g_queue_init (&co->resume_queue);
  g_queue_foreach (&resume_queue, (GFunc)g_coroutine_resume, NULL);
  g_queue_clear (&resume_queue);
}

static gpointer
coroutine_swap (GCoroutine *from, GCoroutine *to, gpointer data)
{
  GCoroutineAction ret;

  to->data = data;
  ret = _g_coroutine_switch (from, to, GCOROUTINE_YIELD);

  coroutine_resume_queue (to);

  switch (ret) {
  case GCOROUTINE_YIELD:
    return from->data;
  case GCOROUTINE_TERMINATE:
    data = to->data;
    g_coroutine_unref (to);
    return data;
  default:
    abort ();
  }
}

/**
 * g_coroutine_new:
 * @func: a function to execute in the new coroutine
 *
 * This function creates a new coroutine. After calling
 * g_coroutine_resume(). the coroutine will run until @func returns or
 * until g_coroutine_yield() is called.
 *
 * If the coroutine can not be created the program aborts.
 *
 * To free the struct returned by this function, use
 * g_coroutine_unref().
 *
 * Returns: the new #GCoroutine
 **/
GCoroutine *
g_coroutine_new (GCoroutineFunc func)
{
  GCoroutine *co;

  g_return_val_if_fail (func != NULL, NULL);

  co = _g_coroutine_new ();
  co->func = func;
  co->ref_count = 1;
  g_queue_init (&co->resume_queue);

  return co;
}

/**
 * g_coroutine_ref:
 * @coroutine: a #GCoroutine
 *
 * Increase the reference count on coroutine.
 *
 * Returns: a new reference to @coroutine
**/
GCoroutine *
g_coroutine_ref (GCoroutine *co)
{
  g_return_val_if_fail (co != NULL, NULL);

  g_atomic_int_inc (&co->ref_count);

  return co;
}

/**
 * g_coroutine_unref:
 * @coroutine: a #GCoroutine
 *
 * Decrease the reference count on coroutine, possibly freeing all
 * resources associated with it.
 *
 * Note that each coroutine holds a reference to its GCoroutine while
 * it is running, so it is safe to drop your own reference to it if
 * you don't need it anymore.
 *
 **/
void
g_coroutine_unref (GCoroutine *co)
{
  g_return_if_fail (co != NULL);

  if (g_atomic_int_dec_and_test (&co->ref_count))
    {
      g_warn_if_fail (g_queue_is_empty (&co->resume_queue));
      _g_coroutine_free (co);
    }
}

/**
 * g_coroutine_resumable:
 * @coroutine: a #GCoroutine
 *
 * This function tests if the @coroutine can be resumed. A coroutine
 * can be resumed if it isn't already resumed and its function is not
 * completed or returned.
 *
 * Returns: #TRUE if the coroutine can be resumed
 **/
gboolean
g_coroutine_resumable (GCoroutine *co)
{
  g_return_val_if_fail (co != NULL, FALSE);

  return co->caller == NULL;
}

/**
 * g_coroutine_resume:
 * @coroutine: a #GCoroutine
 * @data: an argument to supply to the coroutine
 *
 * Enter and resume the context of execution of @coroutine. The
 * coroutine will execute until it returns or yields with
 * g_coroutine_yield().
 *
 * An implicit reference is taken when entering the @coroutine for the
 * first time. This reference is dropped when the coroutine function
 * returns.
 *
 * Returns: the argument returned by the coroutine
 **/
gpointer
g_coroutine_resume (GCoroutine *co, gpointer data)
{
  GCoroutine *self = g_coroutine_self ();

  g_return_val_if_fail (co != NULL, NULL);
  g_return_val_if_fail (co->caller == NULL, NULL);

  co->caller = self;
  return coroutine_swap (self, co, data);
}

/**
 * g_coroutine_yield:
 * @data: an argument to return to the caller context
 *
 * Yields control back to the caller of g_coroutine_resume().
 *
 * Returns: the argument supplied by the caller in
 * g_coroutine_resume()
 **/
gpointer
g_coroutine_yield (gpointer data) G_COROUTINE_FUNC
{
  GCoroutine *self = g_coroutine_self ();
  GCoroutine *to = self->caller;

  g_return_val_if_fail (to != NULL, NULL);

  self->caller = NULL;
  return coroutine_swap (self, to, data);
}

/**
 * g_coroutine_self:
 *
 * This functions returns the #GCoroutine corresponding to the current
 * coroutine. Note that this function does not increase the reference
 * count of the returned struct.
 *
 * Returns: the #GCoroutine representing the current coroutine
 **/
GCoroutine *
g_coroutine_self (void) G_COROUTINE_FUNC
{
  return _g_coroutine_self ();
}

/**
 * g_in_coroutine:
 *
 * This function returns %TRUE if the current context of execution is
 * from a coroutine.
 *
 * Returns: %TRUE if called from a coroutine context
 **/
gboolean
g_in_coroutine (void)
{
  return _g_in_coroutine ();
}

/**
 * GCoQueue:
 *
 * The #GCoQueue struct is an opaque data structure to queue
 * coroutines. It provides the fundamental primitives on which
 * coroutine locks are built.
 */

/**
 * g_co_queue_init:
 * @queue: a #GCoQueue
 *
 * Initializes a #GCoQueue so that it can be used.
 **/
void
g_co_queue_init (GCoQueue *q)
{
  g_return_if_fail (q != NULL);

  g_queue_init (&q->queue);
}

/**
 * g_co_queue_yield:
 * @queue: a #GCoQueue
 * @data: an argument to return to the caller context
 *
 * Yields control back to the caller of g_coroutine_resume().
 *
 * Returns: the argument supplied by the caller in
 * g_coroutine_resume()
 **/
gpointer
g_co_queue_yield (GCoQueue *q, gpointer data) G_COROUTINE_FUNC
{
  g_return_val_if_fail (q != NULL, NULL);
  g_return_val_if_fail (g_in_coroutine (), NULL);

  g_queue_push_tail (&q->queue, g_coroutine_self ());
  return g_coroutine_yield (data);
}

/**
 * g_co_queue_schedule:
 * @queue: a #GCoQueue
 * @n: number of coroutines to schedule
 *
 * Pop @n coroutines from the @queue and schedule them for execution.
 *
 * Returns: the number of coroutines scheduled
 **/
gint
g_co_queue_schedule (GCoQueue *q, gint n) G_COROUTINE_FUNC
{
  GCoroutine *self = g_coroutine_self ();
  gint i;

  g_return_val_if_fail (q != NULL, -1);
  g_return_val_if_fail (n >= -1, -1);

  for (i = 0; (n == -1 || i < n) && !g_queue_is_empty (&q->queue); i++)
    {
      g_queue_push_tail (&self->resume_queue, g_queue_pop_head (&q->queue));
    }

  return i;
}

/**
 * g_co_queue_resume_head:
 * @queue: a #GCoQueue
 * @data: an argument to return to the caller context
 *
 * Resume the first coroutine from the @queue.
 *
 * Returns: the argument returned by the head coroutine
 **/
gpointer
g_co_queue_resume_head (GCoQueue *q, gpointer data)
{
  GCoroutine *co;

  g_return_val_if_fail (q != NULL, NULL);
  g_return_val_if_fail (!g_queue_is_empty (&q->queue), NULL);

  co = g_queue_pop_head (&q->queue);
  g_return_val_if_fail (co != NULL, NULL);

  return g_coroutine_resume (co, data);
}

/**
 * g_co_queue_is_empty:
 * @queue: a #GCoQueue
 *
 * Returns %TRUE if the queue is empty.
 *
 * Returns: %TRUE if the queue is empty
 **/
gboolean
g_co_queue_is_empty (GCoQueue *q)
{
  g_return_val_if_fail (q != NULL, FALSE);

  return g_queue_is_empty (&q->queue);
}

/**
 * GCoMutex:
 *
 * The #GCoMutex struct is an opaque data structure to represent a
 * mutex (mutual exclusion).  It can be used to protect data against
 * shared access. It is similar to #GMutex applied to coroutines.
*/

/**
 * g_co_mutex_init:
 * @mutex: a #GCoMutex
 *
 * Initializes a #GCoMutex so that it can be used.
 **/
void
g_co_mutex_init (GCoMutex *mutex)
{
  g_return_if_fail (mutex != NULL);

  memset (mutex, 0, sizeof(*mutex));
  mutex->locked = FALSE;
}

/**
 * g_co_mutex_lock:
 * @mutex: a #GCoMutex
 *
 * Locks @mutex. If @mutex is already locked by another coroutine, the
 * current coroutine will yield %NULL to the caller coroutine until
 * @mutex is unlocked by the other coroutine.
 *
 **/
void
g_co_mutex_lock (GCoMutex *mutex) G_COROUTINE_FUNC
{
  g_return_if_fail (mutex != NULL);

  while (mutex->locked)
    {
      gpointer data = g_co_queue_yield (&mutex->queue, NULL);
      g_warn_if_fail (data == NULL);
    }

  mutex->locked = TRUE;
}

/**
 * g_co_mutex_unlock:
 * @mutex: a #GCoMutex
 *
 * Unlocks @mutex. If another coroutine is blocking in a
 * g_co_mutex_lock() call for @mutex, it will become unblocked and can
 * lock @mutex itself.
 *
 **/
void
g_co_mutex_unlock (GCoMutex *mutex) G_COROUTINE_FUNC
{
  g_return_if_fail (mutex != NULL);
  g_return_if_fail (mutex->locked);

  mutex->locked = FALSE;
  g_co_queue_schedule (&mutex->queue, 1);
}

/**
 * GCoRWLock:
 *
 * The #GCoRWLock struct is an opaque data structure to represent a reader-writer lock.
 *
 * It is similar to a #GCoMutex in that it allows multiple coroutines
 * to coordinate access to a shared resource.
 *
 * The difference to a mutex is that a reader-writer lock
 * discriminates between read-only ('reader') and full ('writer')
 * access. While only one coroutine at a time is allowed write access
 * (by holding the 'writer' lock via g_co_rw_lock_writer_lock()),
 * multiple coroutines can gain simultaneous read-only access (by
 * holding the 'reader' lock via g_co_rw_lock_reader_lock()).
 */

/**
 * g_co_rw_lock_init:
 * @lock: a #GCoRWLock
 *
 * Initializes a #GCoRWLock so that it can be used.
 **/
void
g_co_rw_lock_init (GCoRWLock *lock)
{
  g_return_if_fail (lock != NULL);

  memset (lock, 0, sizeof(*lock));
  g_co_queue_init (&lock->queue);
}

/**
 * g_co_rw_lock_reader_lock:
 * @lock: a #GCoRWLock
 *
 * Obtain a read lock on @lock. If another coroutine currently holds
 * the write lock on @lock or blocks waiting for it, the current
 * coroutine will yield %NULL to the caller of the current
 * coroutine. Read locks can be taken recursively.
 **/
void
g_co_rw_lock_reader_lock (GCoRWLock *lock) G_COROUTINE_FUNC
{
  g_return_if_fail (lock != NULL);

  while (lock->writer)
    {
      gpointer data = g_co_queue_yield (&lock->queue, NULL);
      g_warn_if_fail (data == NULL);
    }

  lock->reader++;
}

/**
 * g_co_rw_lock_reader_unlock:
 * @lock: a #GCoRWLock
 *
 * Release a read lock.
 **/
void
g_co_rw_lock_reader_unlock (GCoRWLock *lock) G_COROUTINE_FUNC
{
  g_return_if_fail (lock != NULL);
  g_return_if_fail (lock->reader > 0);

  lock->reader--;
  /* wakeup only one waiting writer */
  if (lock->reader == 0)
    g_co_queue_schedule (&lock->queue, 1);
}

/**
 * g_co_rw_lock_writer_lock:
 * @lock: a #GCoRWLock
 *
 * Obtain a write lock. If any coroutine already holds a read or write
 * lock, the current coroutine will yield %NULL until all other
 * coroutines have dropped their locks.
 **/
void
g_co_rw_lock_writer_lock (GCoRWLock *lock) G_COROUTINE_FUNC
{
  g_return_if_fail (lock != NULL);

  while (lock->writer || lock->reader)
    {
      gpointer data = g_co_queue_yield (&lock->queue, NULL);
      g_warn_if_fail (data == NULL);
    }

  lock->writer = TRUE;
}

/**
 * g_co_rw_lock_writer_unlock:
 * @lock: a #GCoRWLock
 *
 * Release a write lock.
 **/
void
g_co_rw_lock_writer_unlock (GCoRWLock *lock) G_COROUTINE_FUNC
{
  g_return_if_fail (lock != NULL);
  g_return_if_fail (lock->writer);

  lock->writer = FALSE;

  g_co_queue_schedule (&lock->queue, -1);
}
