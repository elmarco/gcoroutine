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
