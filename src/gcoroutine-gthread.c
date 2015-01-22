/*
 * GThread coroutine initialization code
 *
 * Copyright (C) 2006  Anthony Liguori <anthony@codemonkey.ws>
 * Copyright (C) 2011  Aneesh Kumar K.V <aneesh.kumar@linux.vnet.ibm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "gcoroutineprivate.h"

typedef struct {
  GCoroutine       base;

  GThread         *thread;
  gboolean         runnable;
  gboolean         free_on_thread_exit;
  GCoroutineAction action;
} GRealCoroutine;

static GMutex coroutine_lock;
static GCond coroutine_cond;

/* Awkwardly, the GPrivate API doesn't provide a way to update the
 * GDestroyNotify handler for the coroutine key dynamically. So instead
 * we track whether or not the CoroutineGThread should be freed on
 * thread exit / coroutine key update using the free_on_thread_exit
 * field.
 */
static void
coroutine_destroy_notify (gpointer data)
{
  GRealCoroutine *co = data;

  if (co && co->free_on_thread_exit)
    {
      g_slice_free (GRealCoroutine, co);
    }
}

static GPrivate coroutine_key = G_PRIVATE_INIT (coroutine_destroy_notify);

static inline GRealCoroutine *
get_coroutine_key (void)
{
  return g_private_get (&coroutine_key);
}

static inline void
set_coroutine_key (GRealCoroutine *co,
                   gboolean free_on_thread_exit)
{
  /* Unlike g_static_private_set() this does not call the GDestroyNotify
   * if the previous value of the key was NULL. Fortunately we only need
   * the GDestroyNotify in the non-NULL key case.
   */
  co->free_on_thread_exit = free_on_thread_exit;
  g_private_replace (&coroutine_key, co);
}

static void
coroutine_wait_runnable_locked (GRealCoroutine *co)
{
    while (!co->runnable)
      {
        g_cond_wait (&coroutine_cond, &coroutine_lock);
      }
}

static void
coroutine_wait_runnable (GRealCoroutine *co)
{
  g_mutex_lock (&coroutine_lock);
  coroutine_wait_runnable_locked (co);
  g_mutex_unlock (&coroutine_lock);
}

static gpointer
coroutine_thread (gpointer opaque)
{
    GRealCoroutine *co = opaque;
    GCoroutine *coro = opaque;

    set_coroutine_key (co, FALSE);
    coroutine_wait_runnable (co);
    g_coroutine_ref (coro);
    co->base.data = co->base.func (co->base.data);
    _g_coroutine_switch (&co->base, co->base.caller, GCOROUTINE_TERMINATE);

    return NULL;
}

GCoroutine *
_g_coroutine_new (void)
{
    GRealCoroutine *co;

    co = g_slice_new0 (GRealCoroutine);
    co->thread = g_thread_new ("coroutine", coroutine_thread, co);

    return (GCoroutine *)co;
}

void
_g_coroutine_free (GCoroutine *co_)
{
  GRealCoroutine *co = (GRealCoroutine *)co_;

  g_thread_join (co->thread);
  g_slice_free (GRealCoroutine, co);
}

GCoroutineAction
_g_coroutine_switch (GCoroutine *from_,
                     GCoroutine *to_,
                     GCoroutineAction action)
{
  GRealCoroutine *from = (GRealCoroutine *)from_;
  GRealCoroutine *to = (GRealCoroutine *)to_;

  g_mutex_lock (&coroutine_lock);
  from->runnable = FALSE;
  from->action = action;
  to->runnable = TRUE;
  to->action = action;
  g_cond_broadcast (&coroutine_cond);
  if (action != GCOROUTINE_TERMINATE)
    {
      coroutine_wait_runnable_locked (from);
    }
  g_mutex_unlock (&coroutine_lock);

  return from->action;
}

GCoroutine *
_g_coroutine_self (void)
{
    GRealCoroutine *co = get_coroutine_key ();

    if (!co) {
        co = g_slice_new0 (GRealCoroutine);
        co->runnable = TRUE;
        set_coroutine_key (co, TRUE);
    }

    return (GCoroutine *)co;
}

gboolean
_g_in_coroutine (void)
{
    GRealCoroutine *co = get_coroutine_key ();

    return co && co->base.caller;
}
