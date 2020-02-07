/*
    cs_par_base.c:

    Copyright (C) 2009: Chris Wilson and John ffitch

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

#include <stdio.h>
#include <stdlib.h>

#include "csoundCore.h"

#include "cs_par_base.h"
static int csp_set_exists(struct set_t *set, void *data);

int csp_thread_index_get(CSOUND *csound)
{
    void *threadId = csound->GetCurrentThreadID();

    int index = 0;
    THREADINFO *current = csound->multiThreadedThreadInfo;

    if (UNLIKELY(current == NULL)) {
      return -1;
    }

    while (current != NULL) {
    // PTHREAD: this should be a class in threads.c to abstract this away
#if defined(HAVE_PTHREAD) && !defined(WIN32)
      if (pthread_equal(*(pthread_t *)threadId, *(pthread_t *)current->threadId)) {
#else
      // FIXME not entirely sure what this should be...
      //if ((DWORD)csoundGetCurrentThreadId () == (DWORD)threadId)
      /*if ((DWORD)current->threadId == (DWORD)threadId) {*/
      if (current->threadId == threadId) {
#endif
        free(threadId);
        return index;
      }
      index++;
      current = current->next;
    }
    return -1;
}


/* **** An implementation of Barriers for MAC that lacks them **** */
#if defined(__MACH__) || defined(ANDROID) || defined(NACL) || defined(__HAIKU__)
/*#define BARRIER_SERIAL_THREAD (-1)

typedef struct {
  pthread_mutex_t mut;
  pthread_cond_t cond;
  unsigned int count, max, iteration;
} barrier_t;
*/
extern int barrier_init(barrier_t *b, void *,unsigned int max);
extern int barrier_destroy(barrier_t *b);
extern int barrier_wait(barrier_t *b);

#ifndef PTHREAD_BARRIER_SERIAL_THREAD
/*#define pthread_barrier_t barrier_t */
#define PTHREAD_BARRIER_SERIAL_THREAD BARRIER_SERIAL_THREAD
#define pthread_barrier_init(barrier, attr, count) \
  barrier_init(barrier,NULL,count)
#define pthread_barrier_destroy barrier_destroy
#define pthread_barrier_wait barrier_wait
#endif

int barrier_init(barrier_t *b, void *dump, unsigned int max)
{
    IGN(dump);
    if (UNLIKELY(max == 0)) return EINVAL;

    if (UNLIKELY(pthread_mutex_init(&b->mut, NULL))) {
      return -1;
    }

    if (UNLIKELY(pthread_cond_init(&b->cond, NULL))) {
      int err = errno;
      pthread_mutex_destroy(&b->mut);
      errno = err;
      return -1;
    }

    b->count = 0;
    b->iteration = 0;
    b->max = max;

    return 0;
}

int barrier_destroy(barrier_t *b)
{
    if (UNLIKELY(b->count > 0)) return EBUSY;

    pthread_cond_destroy(&b->cond);
    pthread_mutex_destroy(&b->mut);

    return 0;
}

/* when barrier is passed, all threads except one return 0 */
int barrier_wait(barrier_t *b)
{
  int ret;
  unsigned int it;

    pthread_mutex_lock(&b->mut);
    b->count++;
    it = b->iteration;
    if (b->count >= b->max) {
      b->count = 0;
      b->iteration++;
      pthread_cond_broadcast(&b->cond);
      ret = BARRIER_SERIAL_THREAD;
    }
    else {
      while (it == b->iteration) pthread_cond_wait(&b->cond, &b->mut);
      ret = 0;
    }
    pthread_mutex_unlock(&b->mut);

    return ret;
}
#endif

/***********************************************************************
 * parallel primitives
 */
void csp_barrier_alloc(CSOUND *csound, void **barrier,
                       int thread_count)
{
    if (UNLIKELY(barrier == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter barrier"));
    if (UNLIKELY(thread_count < 1))
      csound->Die(csound, Str("Invalid Parameter thread_count must be > 0"));

    *barrier = csound->CreateBarrier(thread_count);
    if (UNLIKELY(*barrier == NULL)) {
        csound->Die(csound, Str("Failed to allocate barrier"));
    }
}

void csp_barrier_dealloc(CSOUND *csound, void **barrier)
{
    if (UNLIKELY(barrier == NULL || *barrier == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter barrier"));

    csound->DestroyBarrier(*barrier);
}



/***********************************************************************
 * set data structure
 */

/* static prototypes */
static void set_element_delloc(CSOUND *csound,
                              struct set_element_t **set_element);
static struct set_element_t *set_element_alloc(CSOUND *csound,
                             char *data);
static int set_is_set(struct set_t *set);
#if 0
static int
  set_element_is_set_element(CSOUND *csound,
                             struct set_element_t *set_element);
#endif
struct set_t *csp_set_alloc(CSOUND *csound,
                            set_element_data_eq *ele_eq_func,
                            set_element_data_print *ele_print_func)
{
    struct set_t *p = csound->Malloc(csound, sizeof(struct set_t));
    if (UNLIKELY(p == NULL)) {
      csound->Die(csound, Str("Failed to allocate set"));
    }
    memset(p, 0, sizeof(struct set_t));
    memcpy(p->hdr, SET_HDR, HDR_LEN);
    p->ele_eq_func = ele_eq_func;
    p->ele_print_func = ele_print_func;
    p->cache = NULL;
    //printf("csp_set_alloc: %p\n", p);
    return p;
}

void csp_set_dealloc(CSOUND *csound, struct set_t **set)
{
    struct set_element_t *ele;
    struct set_t *p = *set;
    if (UNLIKELY(set == NULL || *set == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter set"));
    if (UNLIKELY(!set_is_set(*set)))
      csound->Die(csound, Str("Invalid Parameter set not a set"));

    if (p->cache != NULL) csound->Free(csound, p->cache);

    ele = p->head;
    while (ele != NULL) {
      struct set_element_t *next = ele->next;
      set_element_delloc(csound, &ele);
      ele = next;
    }
    //printf("csp_set_dealloc: %p\n", p);
    csound->Free(csound, p);
    *set = NULL;

    return;
}

static struct set_element_t* set_element_alloc(CSOUND *csound,
                             char *data)
{
    struct set_element_t *p;
    if (data == NULL)
      csound->Die(csound, Str("Invalid NULL Parameter data"));

    p = (struct set_element_t*)csound->Malloc(csound, sizeof(struct set_element_t));
    if (UNLIKELY(p == NULL)) {
      csound->Die(csound, Str("Failed to allocate set element"));
    }
    memset(p, 0, sizeof(struct set_element_t));
    memcpy(p->hdr, SET_ELEMENT_HDR, HDR_LEN);
    p->data = cs_strdup(csound, data);

    return p;
}

static void set_element_delloc(CSOUND *csound,
                              struct set_element_t **set_element)
{
    if (UNLIKELY(set_element == NULL || *set_element == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter set_element"));
    csound->Free(csound, *set_element);
    *set_element = NULL;

    return;
}

static int set_is_set(struct set_t *set)
{
    char buf[4];
    if (set == NULL) return 0;
    memcpy(buf, (char *)set, HDR_LEN);
    buf[3] = 0;
    return strcmp(buf, SET_HDR) == 0;
}

#if 0
static int
  set_element_is_set_element(CSOUND *csound,
                             struct set_element_t *set_element)
{
    char buf[4];
    if (set_element == NULL) return 0;
    memcpy(buf, (char *)set_element, HDR_LEN-1);
    buf[3] = 0;
    return strcmp(buf, SET_ELEMENT_HDR) == 0;
}
#endif

struct set_t *csp_set_alloc_string(CSOUND *csound)
{
    return csp_set_alloc(csound,
                  csp_set_element_string_eq,
                  csp_set_element_string_print);
}

int csp_set_element_string_eq(struct set_element_t *ele1,
                              struct set_element_t *ele2)
{
    return strcmp((char *)ele1->data, (char *)ele2->data) == 0;
}

#if 0
int csp_set_element_ptr_eq(struct set_element_t *ele1,
                           struct set_element_t *ele2)
{
    return (ele1->data == ele2->data);
}
#endif

void csp_set_element_string_print(CSOUND *csound,
                                  struct set_element_t *ele)
{
  csound->Message(csound, "%s", (char *)ele->data);
}

void csp_set_element_ptr_print(CSOUND *csound,
                               struct set_element_t *ele)
{
    csound->Message(csound, "%p", ele->data);
}

static void set_update_cache(CSOUND *csound, struct set_t *set)
{
    if (set->cache != NULL) {
      csound->Free(csound, set->cache);
      set->cache = NULL;
    }
    if (set->count > 0) {
      struct set_element_t *ele;
      int ctr = 0;
      set->cache =
        csound->Malloc(csound,
                       sizeof(struct set_element_t *) * set->count);
      ele = set->head;
      while (ele != NULL) {
        set->cache[ctr] = ele;
        ctr++;
        ele = ele->next;
      }
    }
    return;
}

/*
 * if out_set_element is not NULL and the element corresponding to
 * data is not found it will not be changed
 */
static void set_element_get(struct set_t *set,
                           char *data,
                           struct set_element_t **out_set_element)
{
    struct set_element_t *ele = set->head;
    struct set_element_t data_ele = { SET_ELEMENT_HDR, data, 0 };
    while (ele != NULL) {
      if (set->ele_eq_func(ele, &data_ele)) {
        *out_set_element = ele;
        break;
      }
      ele = ele->next;
    }
    return;
}

void csp_set_add(CSOUND *csound, struct set_t *set, void *data)
{
    struct set_element_t *ele = NULL;
#ifdef SET_DEBUG
    if (UNLIKELY(set == NULL))
      csound->Die(csound, "Invalid NULL Parameter set");
    if (UNLIKELY(data == NULL))
      csound->Die(csound, "Invalid NULL Parameter data");
#endif

    if (csp_set_exists(set, data)) {
        return;
    }

    ele = set_element_alloc(csound, data);
    if (set->head == NULL) {
      set->head = ele;
      set->tail = ele;
    }
    else {
      set->tail->next = ele;
      set->tail = ele;
    }
    set->count++;

    set_update_cache(csound, set);

    return;
}

void csp_set_remove(CSOUND *csound, struct set_t *set, void *data)
{
#ifdef SET_DEBUG
    if (UNLIKELY(set == NULL))
      csound->Die(csound, "Invalid NULL Parameter set");
    if (UNLIKELY(data == NULL))
      csound->Die(csound, "Invalid NULL Parameter data");
#endif
    {
      struct set_element_t *ele = set->head, *prev = NULL;
      struct set_element_t data_ele = { SET_ELEMENT_HDR, data, 0 };
      while (ele != NULL) {
        if (set->ele_eq_func(ele, &data_ele)) {
          if (ele == set->head && ele == set->tail) {
            set->head = NULL;
            set->tail = NULL;
          }
          else if (ele == set->head) {
            set->head = ele->next;
          }
          else {
            prev->next = ele->next;
          }
          set_element_delloc(csound, &ele);
          set->count--;
          break;
        }
        prev = ele;
        ele = ele->next;
      }
    }
    set_update_cache(csound, set);

    return;
}

static int csp_set_exists(struct set_t *set, void *data)
{
    struct set_element_t *ele = NULL;
/* #ifdef SET_DEBUG */
/*     if (UNLIKELY(set == NULL)) */
/*       csound->Die(csound, "Invalid NULL Parameter set"); */
/*     if (UNLIKELY(data == NULL)) */
/*       csound->Die(csound, "Invalid NULL Parameter data"); */
/* #endif */
    set_element_get(set, data, &ele);

    return (ele == NULL ? 0 : 1);
}

void csp_set_print(CSOUND *csound, struct set_t *set)
{
    struct set_element_t *ele;
#ifdef SET_DEBUG
    if (UNLIKELY(set == NULL))
      csound->Die(csound, "Invalid NULL Parameter set");
    if (UNLIKELY(!set_is_set(set)))
      csound->Die(csound, "Invalid Parameter set not a set");
#endif

    ele = set->head;

    csound->Message(csound, "{ ");
    while (ele != NULL) {
      set->ele_print_func(csound, ele);
      if (ele->next != NULL) csound->Message(csound, ", ");
      ele = ele->next;
    }
    csound->Message(csound, " }\n");

    return;
}

inline int csp_set_count(struct set_t *set)
{
/* #ifdef SET_DEBUG */
/*     if (UNLIKELY(set == NULL)) */
/*       csound->Die(csound, "Invalid NULL Parameter set"); */
/*     if (UNLIKELY(!set_is_set(set))) */
/*       csound->Die(csound, "Invalid Parameter set not a set"); */
/* #endif */

    return set->count;
}

/* 0 indexed */
// FIXME inlining breaks linkage for MSVC
inline static void* csp_set_get_num(struct set_t *set, int num)
{
/* #ifdef SET_DEBUG */
/*     if (UNLIKELY(set == NULL)) */
/*       csound->Die(csound, "Invalid NULL Parameter set"); */
/*     if (UNLIKELY(!set_is_set(set))) */
/*       csound->Die(csound, "Invalid Parameter set not a set"); */
/*     if (UNLIKELY(um >= set->count)) */
/*       csound->Die(csound, "Invalid Parameter num is out of bounds"); */
/*     if (UNLIKELY(data == NULL)) */
/*       csound->Die(csound, "Invalid NULL Parameter data"); */
/* #endif */

    return set->cache[num]->data;

    /* if (set->cache != NULL) { */

    /* } */
    /* else { */
    /*   int ctr = 0; */
    /*   struct set_element_t *ele = set->head; */
    /*   while (ctr < num && ele != NULL) { */
    /*     ctr++; */
    /*     ele = ele->next; */
    /*   } */
    /*   if (ctr == num && ele != NULL) { */
    /*     *data = ele->data; */
    /*   } */
    /* } */

}

struct set_t *csp_set_union(CSOUND *csound, struct set_t *first,
                  struct set_t *second)
{
    int ctr = 0;
    int first_len;
    int second_len;
    struct set_t *result;
#ifdef SET_DEBUG
    if (UNLIKELY(first == NULL))
      csound->Die(csound, "Invalid NULL Parameter first");
    if (UNLIKELY(!set_is_set(first)))
      csound->Die(csound, "Invalid Parameter set not a first");
    if (UNLIKELY(second == NULL))
      csound->Die(csound, "Invalid NULL Parameter second");
    if (UNLIKELY(!set_is_set(second)))
      csound->Die(csound, "Invalid Parameter set not a second");
    if (UNLIKELY(result == NULL))
      csound->Die(csound, "Invalid NULL Parameter result");
    if (UNLIKELY(first->ele_eq_func != second->ele_eq_func))
      csound->Die(csound,
                  "Invalid sets for comparison (different equality)");
#endif

    result = csp_set_alloc(csound,
                  first->ele_eq_func, first->ele_print_func);

    first_len = csp_set_count(first);
    second_len = csp_set_count(second);

    while (ctr < first_len) {
      void *data = NULL;
      data = csp_set_get_num(first, ctr);
      csp_set_add(csound, result, data);
      ctr++;
    }

    ctr = 0;
    while (ctr < second_len) {
      void *data = NULL;
      data = csp_set_get_num(second, ctr);
      csp_set_add(csound, result, data);
      ctr++;
    }
    return result;
}

struct set_t *csp_set_intersection(CSOUND *csound, struct set_t *first,
                         struct set_t *second)
{
    int ctr = 0;
    int first_len;
    struct set_t *result;
#ifdef SET_DEBUG
    if (UNLIKELY(first == NULL))
      csound->Die(csound, "Invalid NULL Parameter first");
    if (UNLIKELY(!set_is_set(first)))
      csound->Die(csound, "Invalid Parameter set not a first");
    if (UNLIKELY(second == NULL))
      csound->Die(csound, "Invalid NULL Parameter second");
    if (UNLIKELY(!set_is_set(second)))
      csound->Die(csound, "Invalid Parameter set not a second");
    if (UNLIKELY(result == NULL))
      csound->Die(csound, "Invalid NULL Parameter result");
    if (UNLIKELY(first->ele_eq_func != second->ele_eq_func))
      csound->Die(csound,
                  "Invalid sets for comparison (different equality)");
#endif

    result = csp_set_alloc(csound,
                            first->ele_eq_func, first->ele_print_func);

    first_len = csp_set_count(first);

    while (ctr < first_len) {
      void *data = NULL;
      data = csp_set_get_num(first, ctr);
      if (csp_set_exists(second, data)) {
        csp_set_add(csound, result, data);
      }
      ctr++;
    }

    return result;
}
