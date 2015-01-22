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
 */
#include <glib.h>
#include <gcoroutine.h>

/*
 * Check that g_in_coroutine() works
 */

static gpointer
verify_in_coroutine (gpointer data) G_COROUTINE_FUNC
{
  g_assert (g_in_coroutine ());

  return NULL;
}

static void
test_in_coroutine (void)
{
  GCoroutine *coroutine;

  g_assert (!g_in_coroutine ());

  coroutine = g_coroutine_new (verify_in_coroutine);
  g_assert (g_coroutine_resumable (coroutine));
  g_coroutine_resume (coroutine, NULL);
  g_assert (!g_coroutine_resumable (coroutine));
  g_coroutine_unref (coroutine);

  g_assert (!g_in_coroutine ());
}

/*
 * Check that g_coroutine_self() works
 */

static gpointer
verify_self (gpointer data) G_COROUTINE_FUNC
{
  GCoroutine *coroutine = data;

  g_assert (coroutine == g_coroutine_self ());

  return NULL;
}

static void
test_self (void)
{
  GCoroutine *coroutine;

  g_assert (g_coroutine_self ());
  coroutine = g_coroutine_new (verify_self);
  g_coroutine_resume (coroutine, coroutine);
  g_assert (g_coroutine_self () != coroutine);
  g_coroutine_unref (coroutine);
}

/*
 * Check that coroutines may nest multiple levels
 */

typedef struct {
  guint n_enter;   /* num coroutines entered */
  guint n_return;  /* num coroutines returned */
  guint max;       /* maximum level of nesting */
} NestData;

static gpointer
nest (gpointer data) G_COROUTINE_FUNC
{
  NestData *nd = data;

  nd->n_enter++;

  if (nd->n_enter < nd->max)
    {
      GCoroutine *c = g_coroutine_new (nest);
      g_coroutine_resume (c, nd);
      g_coroutine_unref (c);
    }

  nd->n_return++;
  return NULL;
}

static void
test_nesting (void)
{
  GCoroutine *c;
  NestData nd = {
    .n_enter  = 0,
    .n_return = 0,
    .max      = 128,
  };

  c = g_coroutine_new (nest);
  g_coroutine_resume (c, &nd);
  g_coroutine_unref (c);

  /* Must enter and return from max nesting level */
  g_assert_cmpint (nd.n_enter, ==, nd.max);
  g_assert_cmpint (nd.n_return, ==, nd.max);
}

/*
 * Check that yield/enter transfer control correctly
 */

static gpointer
yield_5_times (gpointer data) G_COROUTINE_FUNC
{
  gboolean *done = data;
  int i;

  for (i = 0; i < 5; i++)
    {
      done = g_coroutine_yield (GINT_TO_POINTER (i));
    }

  *done = TRUE;
  return GINT_TO_POINTER (i);
}

static void
test_yield (void)
{
  GCoroutine *coroutine;
  gpointer ret;
  gboolean done = FALSE;
  int i = 0;

  coroutine = g_coroutine_new (yield_5_times);
  while (1)
    {
      g_assert (g_coroutine_resumable (coroutine));
      ret = g_coroutine_resume (coroutine, &done);
      if (done)
        break;
      g_assert_cmpint (GPOINTER_TO_INT (ret), ==, i);
      i++;
    }

  g_assert_cmpint (i, ==, 5); /* coroutine must yield 5 times */
  g_coroutine_unref (coroutine);
}

/*
 * Check that creation, enter, and return work
 */

static gpointer
set_and_exit (gpointer data) G_COROUTINE_FUNC
{
  gboolean *done = data;

  *done = TRUE;

  return NULL;
}

static void
test_lifecycle (void)
{
  GCoroutine *coroutine;
  gboolean done = FALSE;
  gpointer ret;

  /* Create, enter, and return from coroutine */
  coroutine = g_coroutine_new (set_and_exit);
  g_coroutine_resume (coroutine, &done);
  g_coroutine_unref (coroutine);
  g_assert (done); /* expect done to be true (first time) */

  /* Repeat to check that no state affects this test */
  done = FALSE;
  coroutine = g_coroutine_new (set_and_exit);
  ret = g_coroutine_resume (coroutine, &done);
  g_assert (ret == NULL);
  g_coroutine_unref (coroutine);
  g_assert (done); /* expect done to be true (second time) */
}

static gpointer
yield_done (gpointer data) G_COROUTINE_FUNC
{
    gboolean *done = data;

    g_coroutine_yield (NULL);

    *done = TRUE;
}

static void
test_unref (void)
{
    GCoroutine *coroutine;
    gboolean done = FALSE;

    /* checks no leaking in simple case */
    coroutine = g_coroutine_new (yield_5_times);
    g_coroutine_unref (coroutine);

    /* checks if resume takes an implicit ref */
    coroutine = g_coroutine_new (yield_done);
    g_coroutine_resume (coroutine, &done);
    g_assert (!done);
    g_coroutine_unref (coroutine);

    g_coroutine_resume (coroutine, NULL);
    g_assert (done);
}

/*
 * Lifecycle benchmark
 */

static gpointer
empty_coroutine (gpointer data) G_COROUTINE_FUNC
{
  /* Do nothing */
  return NULL;
}

static void
perf_lifecycle (void)
{
  guint i, max;
  gdouble duration;

  max = 1000000;

  g_test_timer_start ();
  for (i = 0; i < max; i++)
    {
      GCoroutine *c = g_coroutine_new (empty_coroutine);
      g_coroutine_resume (c, NULL);
      g_coroutine_unref (c);
    }
  duration = g_test_timer_elapsed ();

  g_test_message ("Lifecycle %u iterations: %f s\n", max, duration);
}

static void
perf_nesting (void)
{
  GCoroutine *c;
  guint i, maxcycles, maxnesting;
  gdouble duration;

  maxcycles = 10000;
  maxnesting = 1000;

  g_test_timer_start ();
  for (i = 0; i < maxcycles; i++)
    {
      NestData nd = {
        .n_enter  = 0,
        .n_return = 0,
        .max      = maxnesting,
      };
      c = g_coroutine_new (nest);
      g_coroutine_resume (c, &nd);
      g_coroutine_unref (c);
    }

  duration = g_test_timer_elapsed ();

  g_test_message ("Nesting %u iterations of %u depth each: %f s\n",
                  maxcycles, maxnesting, duration);
}

/*
 * Yield benchmark
 */

static gpointer
yield_loop (gpointer data) G_COROUTINE_FUNC
{
  guint *counter = data;

  while ((*counter) > 0)
    {
      (*counter)--;
      g_coroutine_yield (NULL);
    }

  return NULL;
}

static void
perf_yield (void)
{
  GCoroutine *c;
  guint i, maxcycles;
  gdouble duration;

  maxcycles = 100000000;
  i = maxcycles;

  c = g_coroutine_new (yield_loop);

  g_test_timer_start ();
  while (i > 0)
    {
      g_coroutine_resume (c, &i);
    }
  duration = g_test_timer_elapsed ();

  g_test_message ("Yield %u iterations: %f s\n",
                  maxcycles, duration);

  g_coroutine_resume (c, NULL); /* finish */
  g_coroutine_unref (c);
}

static gpointer
co_lock_third (gpointer data) G_COROUTINE_FUNC
{
  gint *seq = data;
  GCoMutex *mutex = g_coroutine_yield (NULL);;

  g_assert_cmpint (*seq, ==, 0);
  g_co_mutex_lock (mutex);
  g_assert_cmpint (*seq, ==, 3);
  g_co_mutex_unlock (mutex);
  *seq = 4;

  return NULL;
}

static gpointer
co_lock_second (gpointer data) G_COROUTINE_FUNC
{
  gint *seq = data;
  GCoMutex *mutex = g_coroutine_yield (NULL);;

  g_assert_cmpint (*seq, ==, 0);
  g_co_mutex_lock (mutex);
  g_assert_cmpint (*seq, ==, 2);
  g_co_mutex_unlock (mutex);
  *seq = 3;

  return NULL;
}

static gpointer
co_lock_first (gpointer data) G_COROUTINE_FUNC
{
  gint *seq = data;
  GCoMutex *mutex = g_coroutine_yield (NULL);;

  g_assert_cmpint (*seq, ==, 0);
  g_assert (!mutex->locked);
  g_co_mutex_lock (mutex);
  g_assert (mutex->locked);
  g_coroutine_yield (NULL);
  g_assert_cmpint (*seq, ==, 0);
  *seq = 1;
  g_co_mutex_unlock (mutex);
  g_assert (!mutex->locked);
  g_assert_cmpint (*seq, ==, 1);
  *seq = 2;

  return NULL;
}

static void
test_mutex (void)
{
  GCoroutine *first, *second, *third;
  GCoMutex mutex;
  gint seq = 0;

  g_co_mutex_init (&mutex);

  first = g_coroutine_new (co_lock_first);
  g_coroutine_resume (first, &seq);
  g_coroutine_resume (first, &mutex);

  second = g_coroutine_new (co_lock_second);
  g_coroutine_resume (second, &seq);
  g_coroutine_resume (second, &mutex);

  third = g_coroutine_new (co_lock_third);
  g_coroutine_resume (third, &seq);
  g_coroutine_resume (third, &mutex);

  g_coroutine_resume (first, NULL);
  g_assert_cmpint (seq, ==, 4);

  g_coroutine_unref (third);
  g_coroutine_unref (second);
  g_coroutine_unref (first);
}

static gpointer
co_wlock (gpointer data) G_COROUTINE_FUNC
{
    gint *seq = data;
    GCoRWLock *lock = g_coroutine_yield (NULL);;

    g_co_rw_lock_writer_lock (lock);
    (*seq)++;
    g_coroutine_yield (NULL);
    g_co_rw_lock_writer_unlock (lock);
    (*seq)++;

    return NULL;
}

static gpointer
co_rlock (gpointer data) G_COROUTINE_FUNC
{
    gint *seq = data;
    GCoRWLock *lock = g_coroutine_yield (NULL);;

    g_co_rw_lock_reader_lock (lock);
    (*seq)++;
    g_coroutine_yield (NULL);
    g_co_rw_lock_reader_unlock (lock);
    (*seq)++;

    return NULL;
}

static void
test_rwlock (void)
{
    GCoroutine *wlock, *rlock1, *rlock2;
    GCoRWLock lock;
    gint seq = 0;

    g_co_rw_lock_init (&lock);

    /* writer locks */
    wlock = g_coroutine_new (co_wlock);
    g_coroutine_resume (wlock, &seq);
    g_coroutine_resume (wlock, &lock);
    g_assert_cmpint (seq, ==, 1);

    /* readers waiting */
    rlock1 = g_coroutine_new (co_rlock);
    g_coroutine_resume (rlock1, &seq);
    g_coroutine_resume (rlock1, &lock);

    rlock2 = g_coroutine_new (co_rlock);
    g_coroutine_resume (rlock2, &seq);
    g_coroutine_resume (rlock2, &lock);
    g_assert_cmpint (seq, ==, 1);

    /* writer release lock */
    g_coroutine_resume (wlock, NULL);
    /* readers continue */
    g_assert_cmpint (seq, ==, 4);
    g_coroutine_unref (wlock);

    /* writer locks, waiting */
    wlock = g_coroutine_new (co_wlock);
    g_coroutine_resume (wlock, &seq);
    g_coroutine_resume (wlock, &lock);
    g_assert_cmpint (seq, ==, 4);

    /* readers release */
    g_coroutine_resume (rlock1, NULL);
    g_coroutine_resume (rlock2, NULL);
    /* writer get lock */
    g_assert_cmpint (seq, ==, 7);
    g_coroutine_resume (wlock, NULL);

    g_coroutine_unref (rlock2);
    g_coroutine_unref (rlock1);
    g_coroutine_unref (wlock);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/basic/lifecycle", test_lifecycle);
  g_test_add_func ("/basic/unref", test_unref);
  g_test_add_func ("/basic/yield", test_yield);
  g_test_add_func ("/basic/nesting", test_nesting);
  g_test_add_func ("/basic/self", test_self);
  g_test_add_func ("/basic/in_coroutine", test_in_coroutine);
  if (g_test_perf ())
    {
      g_test_add_func ("/perf/lifecycle", perf_lifecycle);
      g_test_add_func ("/perf/nesting", perf_nesting);
      g_test_add_func ("/perf/yield", perf_yield);
    }

  g_test_add_func ("/lock/mutex", test_mutex);
  g_test_add_func ("/lock/rwlock", test_rwlock);

  return g_test_run ();
}
