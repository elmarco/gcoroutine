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

#ifndef __G_COROUTINEPRIVATE_H__
#define __G_COROUTINEPRIVATE_H__

#include "gcoroutine.h"

struct _GCoroutine {
  gint                    ref_count;
  GCoroutineFunc          func;
  gpointer                data;
  GCoroutine             *caller;
  GQueue                  resume_queue;
};

typedef enum {
  GCOROUTINE_YIELD      = 1,
  GCOROUTINE_TERMINATE  = 2,
} GCoroutineAction;

GCoroutineAction          _g_coroutine_switch         (GCoroutine *from_,
                                                       GCoroutine *to_,
                                                       GCoroutineAction action);

GCoroutine *              _g_coroutine_new            (void);
void                      _g_coroutine_free           (GCoroutine *co_);
gboolean                  _g_in_coroutine             (void);
GCoroutine *              _g_coroutine_self           (void);

#endif /* __G_COROUTINEPRIVATE_H__ */
