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

/* XXX Is there a nicer way to disable glibc's stack check for longjmp? */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "gcoroutineprivate.h"
#include "valgrind.h"

#include <errno.h>
#include <setjmp.h>
#include <ucontext.h>

typedef struct {
  GCoroutine       base;

  gpointer         stack;
  sigjmp_buf       env;
  unsigned int     valgrind_stack_id;
} GRealCoroutine;

/**
 * Per-thread coroutine bookkeeping
 */
typedef struct {
  /* Currently executing coroutine */
  GCoroutine    *current;

  /* The default coroutine */
  GRealCoroutine leader;
} GCoroutineThreadState;

static GPrivate thread_state_key = G_PRIVATE_INIT (g_free);

static GCoroutineThreadState *
coroutine_get_thread_state (void)
{
  GCoroutineThreadState *s;

  s = g_private_get (&thread_state_key);

  if (s == NULL)
    {
      s = g_new0 (GCoroutineThreadState, 1);
      s->current = (GCoroutine *) &s->leader;
      g_private_set (&thread_state_key, s);
    }

  return s;
}

/*
 * va_args to makecontext() must be type 'int', so passing
 * the pointer we need may require several int args. This
 * union is a quick hack to let us do that
 */
union cc_arg {
  gpointer p;
  int      i[2];
};
G_STATIC_ASSERT(sizeof(gpointer) <= sizeof(int) * 2);

GCoroutineAction
_g_coroutine_switch (GCoroutine *from_, GCoroutine *to_,
                     GCoroutineAction action)
{
  GRealCoroutine *from = (GRealCoroutine *)from_;
  GRealCoroutine *to = (GRealCoroutine *)to_;
  GCoroutineThreadState *s = coroutine_get_thread_state ();
  gint ret;

  s->current = to_;

  ret = sigsetjmp (from->env, 0);
  if (ret == 0)
    {
      siglongjmp (to->env, action);
    }

  return ret;
}

static void
coroutine_trampoline (int i0, int i1)
{
  union cc_arg arg;
  GRealCoroutine *realco;
  GCoroutine *co;

  arg.i[0] = i0;
  arg.i[1] = i1;
  realco = arg.p;
  co = arg.p;

  /* Initialize longjmp environment and switch back the caller */
  if (!sigsetjmp (realco->env, 0))
    {
      siglongjmp (*(sigjmp_buf *)co->data, 1);
    }

  while (1)
    {
      g_coroutine_ref (co);
      co->data = co->func (co->data);
      _g_coroutine_switch (co, co->caller, GCOROUTINE_TERMINATE);
    }
}

GCoroutine *
_g_coroutine_new (void)
{
  GRealCoroutine *co;
  const size_t stack_size = 1 << 20;
  ucontext_t old_uc, uc;
  sigjmp_buf old_env;
  union cc_arg arg = { 0 };

  /* The ucontext functions preserve signal masks which incurs a
   * system call overhead.  sigsetjmp(buf, 0)/siglongjmp() does not
   * preserve signal masks but only works on the current stack.
   * Since we need a way to create and switch to a new stack, use
   * the ucontext functions for that but sigsetjmp()/siglongjmp() for
   * everything else.
   */
  if (getcontext (&uc) == -1)
    {
      g_error ("getcontext failed: %s", g_strerror (errno));
    }

  co = g_slice_new0 (GRealCoroutine);
  co->stack = g_malloc (stack_size);
  co->base.data = &old_env; /* stash away our jmp_buf */
  uc.uc_link = &old_uc;
  uc.uc_stack.ss_sp = co->stack;
  uc.uc_stack.ss_size = stack_size;
  uc.uc_stack.ss_flags = 0;

  co->valgrind_stack_id =
    VALGRIND_STACK_REGISTER (co->stack, co->stack + stack_size);

  arg.p = co;
  makecontext (&uc, (void (*)(void))coroutine_trampoline,
               2, arg.i[0], arg.i[1]);

  /* swapcontext() in, siglongjmp() back out */
  if (!sigsetjmp (old_env, 0))
    {
      swapcontext (&old_uc, &uc);
    }

  return (GCoroutine *)co;
}

#ifdef CONFIG_PRAGMA_DIAGNOSTIC_AVAILABLE
/* Work around an unused variable in the valgrind.h macro... */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
static inline void
valgrind_stack_deregister (GRealCoroutine *co)
{
  VALGRIND_STACK_DEREGISTER (co->valgrind_stack_id);
}
#ifdef CONFIG_PRAGMA_DIAGNOSTIC_AVAILABLE
#pragma GCC diagnostic pop
#endif

void
_g_coroutine_free (GCoroutine *co_)
{
  GRealCoroutine *co = (GRealCoroutine *)co_;

  valgrind_stack_deregister (co);

  g_free (co->stack);
  g_slice_free (GRealCoroutine, co);
}

GCoroutine *
_g_coroutine_self (void)
{
  GCoroutineThreadState *s = coroutine_get_thread_state ();

  return s->current;
}

gboolean
_g_in_coroutine (void)
{
  GCoroutineThreadState *s = coroutine_get_thread_state ();

  return s->current->caller != NULL;
}
