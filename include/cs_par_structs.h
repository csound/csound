/*
    cs_par_structs.h:

    Copyright (C) 2011 John ffitch and Chris Wilson
                  2013 John ffitch and Martin Brain

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef __CS_PAR_DISPATCH_H
#define __CS_PAR_DISPATCH_H

/* global variables lock support */
struct global_var_lock_t;

struct instr_semantics_t;

/* New model */

/* Current memory subsystems work at the granularity of cache lines.
 * Thus concurrent accesses to multiple objects within a cache line
 * can cause additional conflicts and memory subsystem traffic.
 * The best way to avoid this is to pad the structures so that they don't
 * share a cache line.  64 bytes seems to be becoming the usual cache line
 * size.  If the actual line is smaller, it will waste a little memory but
 * shouldn't affect the sharing.  If the actual line is larger then it may
 * cause a few conflicts and thus a performance hit.  Unless you are
 * using a very unusual machine and you happen to know it's cache line size
 * then leave this as 64, although you can try 32 or maybe even 16.
 */
#define CONCURRENTPADDING 64

typedef int taskID;

/* Each task has a status */
enum state { WAITING = 3,          /* Dependencies have not been finished */
             AVAILABLE = 2,        /* Dependencies met, ready to be run */
             INPROGRESS = 1,       /* Has been started */
             DONE = 0 };           /* Has been completed */

typedef struct _stateWithPadding {
  enum state s;
  uint8_t padding [(CONCURRENTPADDING - sizeof(enum state)) / sizeof(uint8_t)];
} stateWithPadding;

/* Sets of prerequiste tasks for each task */
typedef struct _watchList {
  taskID id;
  struct _watchList *next;
  uint8_t padding [(CONCURRENTPADDING -
                    (sizeof(taskID) +
                     sizeof(struct _watchList *))) / sizeof(uint8_t)];
} watchList;

#endif
