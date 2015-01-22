/*
 * Win32 coroutine initialization code
 *
 * Copyright (c) 2011 Kevin Wolf <kwolf@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include <windows.h>

#include "gcoroutineprivate.h"

typedef struct {
  GCoroutine       base;

  LPVOID           fiber;
  GCoroutineAction  action;
} GRealCoroutine;

static __thread GRealCoroutine leader;
static __thread GCoroutine *current;

GCoroutineAction
_g_coroutine_switch (GCoroutine      *from_,
                     GCoroutine      *to_,
                     GCoroutineAction action)
{
  GRealCoroutine *from = (GRealCoroutine*)from_;
  GRealCoroutine *to = (GRealCoroutine*)to_;

  current = to_;

  to->action = action;
  SwitchToFiber (to->fiber);
  return from->action;
}

static void CALLBACK
coroutine_trampoline (void *co_)
{
  GCoroutine *co = co_;

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
  const size_t stack_size = 1 << 20;
  GRealCoroutine *co;

  co = g_slice_new0 (GRealCoroutine);
  co->fiber = CreateFiber (stack_size, coroutine_trampoline, co);

  return (GCoroutine*)co;
}

void
_g_coroutine_free (GCoroutine *co_)
{
  GRealCoroutine *co = (GRealCoroutine*)co_;

  DeleteFiber (co->fiber);
  g_slice_free (GRealCoroutine, co);
}

GCoroutine *
_g_coroutine_self (void)
{
  if (!current)
    {
      current = (GCoroutine*)&leader;
      leader.fiber = ConvertThreadToFiber (NULL);
    }

  return current;
}

gboolean
_g_in_coroutine (void)
{
  return current && current->caller;
}
