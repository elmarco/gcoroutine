/*
 * GLib coroutines (adapted from QEMU coroutines)
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2011-2014 Red Hat, Inc.
 *
 * Authors:
 *  Stefan Hajnoczi    <stefanha@linux.vnet.ibm.com>
 *  Kevin Wolf         <kwolf@redhat.com>
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

#ifndef __G_COROUTINE_H__
#define __G_COROUTINE_H__

#include <glib.h>

#define GCOROUTINE_H_INSIDE

#include "gcoroutine-version-macros.h"

G_BEGIN_DECLS

#define G_COROUTINE_FUNC

typedef struct _GCoroutine GCoroutine;

typedef gpointer      (*GCoroutineFunc)      (gpointer data) G_COROUTINE_FUNC;


GCOROUTINE_AVAILABLE_IN_1_0
GCoroutine *           g_coroutine_new       (GCoroutineFunc func);
GCOROUTINE_AVAILABLE_IN_1_0
GCoroutine *           g_coroutine_ref       (GCoroutine    *coroutine);
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_coroutine_unref     (GCoroutine    *coroutine);
GCOROUTINE_AVAILABLE_IN_1_0
gboolean               g_coroutine_resumable (GCoroutine    *coroutine);
GCOROUTINE_AVAILABLE_IN_1_0
gpointer               g_coroutine_resume    (GCoroutine    *coroutine,
                                              gpointer       data);
GCOROUTINE_AVAILABLE_IN_1_0
gpointer               g_coroutine_yield     (gpointer       data) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
GCoroutine *           g_coroutine_self      (void) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
gboolean               g_in_coroutine        (void);

typedef struct _GCoQueue GCoQueue;
struct _GCoQueue {
  /*< private >*/
  GQueue queue;
};


GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_queue_init       (GCoQueue      *queue);
GCOROUTINE_AVAILABLE_IN_1_0
gpointer               g_co_queue_yield      (GCoQueue      *queue,
                                              gpointer       data) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
gboolean               g_co_queue_schedule   (GCoQueue      *queue,
                                              gint           n) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
gboolean               g_co_queue_is_empty   (GCoQueue      *queue);
GCOROUTINE_AVAILABLE_IN_1_0
gpointer               g_co_queue_resume_head(GCoQueue      *queue,
                                              gpointer       data);

typedef struct _GCoMutex GCoMutex;
struct _GCoMutex {
  /*< private >*/
  GCoQueue queue;
  gboolean locked;
};

GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_mutex_init       (GCoMutex      *mutex);
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_mutex_lock       (GCoMutex      *mutex) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_mutex_unlock     (GCoMutex      *mutex) G_COROUTINE_FUNC;

typedef struct _GCoRWLock GCoRWLock;
struct _GCoRWLock {
    /*< private >*/
    GCoQueue queue;
    gint reader;
    gboolean writer;
};

GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_rw_lock_init         (GCoRWLock *lock);
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_rw_lock_reader_lock  (GCoRWLock *lock) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_rw_lock_reader_unlock(GCoRWLock *lock) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_rw_lock_writer_lock  (GCoRWLock *lock) G_COROUTINE_FUNC;
GCOROUTINE_AVAILABLE_IN_1_0
void                   g_co_rw_lock_writer_unlock(GCoRWLock *lock) G_COROUTINE_FUNC;

G_END_DECLS


#endif /* __G_COROUTINE_H__ */
