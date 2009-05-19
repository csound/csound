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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>

#include "csoundCore.h"

#include "cs_par_base.h"

int csp_thread_index_get(CSOUND *csound)
{
#ifndef mac_classic
    void *threadId = csound->GetCurrentThreadID();

    int index = 0;
    THREADINFO *current = csound->multiThreadedThreadInfo;

    if(current == NULL) {
      return -1;
    }

    while(current != NULL) {
      if (pthread_equal(*(pthread_t *)threadId,
                        *(pthread_t *)current->threadId)) {
        free(threadId);
        return index;
      }
      index++;
      current = current->next;
    }
#endif
    return -1;
}

/***********************************************************************
 * parallel primitives
 */
void csp_barrier_alloc(CSOUND *csound, pthread_barrier_t **barrier,
                       int thread_count)
{
    if (barrier == NULL) csound->Die(csound, "Invalid NULL Parameter barrier");
    if (thread_count < 1)
      csound->Die(csound, "Invalid Parameter thread_count must be > 0");

    *barrier = (pthread_barrier_t *)csound->Malloc(csound, sizeof(pthread_barrier_t));
    if (*barrier == NULL) {
        csound->Die(csound, "Failed to allocate barrier");
    }
    fprintf(stderr, "Calling barrier init(%p, NULL, %d)\n", *barrier, thread_count);
    pthread_barrier_init(*barrier, NULL, thread_count);
}

void csp_barrier_dealloc(CSOUND *csound, pthread_barrier_t **barrier)
{
    if (barrier == NULL || *barrier == NULL)
      csound->Die(csound, "Invalid NULL Parameter barrier");

    pthread_barrier_destroy(*barrier);
}

void csp_barrier_wait(CSOUND *csound, pthread_barrier_t *barrier)
{
    TRACE_1("Barrier Wait %p\n", barrier);
   pthread_barrier_wait(barrier);
}

/***********************************************************************
 * semaphore
 */

void csp_semaphore_alloc(CSOUND *csound, sem_t **sem, int max_threads)
{
    if (sem == NULL) csound->Die(csound, "Invalid NULL Parameter sem");

    *sem = csound->Malloc(csound, sizeof(sem_t));
    if (*sem == NULL) {
      csound->Die(csound, "Failed to allocate barrier");
    }
    sem_init(*sem, 0, (unsigned int)max_threads);
}

void csp_semaphore_dealloc(CSOUND *csound, sem_t **sem)
{
    if (sem == NULL || *sem == NULL)
      csound->Die(csound, "Invalid NULL Parameter sem");
    sem_destroy(*sem);
    csound->Free(csound, *sem);
    *sem = NULL;
}

void csp_semaphore_wait(CSOUND *csound, sem_t *sem)
{
    if (UNLIKELY(sem == NULL)) csound->Die(csound, "Invalid NULL Parameter sem");

    TRACE_1("[%i] wait:\n", csp_thread_index_get(csound));
    sem_wait(sem);
    TRACE_1("[%i] continue:\n", csp_thread_index_get(csound));
}

void csp_semaphore_grow(CSOUND *csound, sem_t *sem)
{
    /* This should not happen! */
    csound->Die(csound, "csp_semaphore_grow");
}

void csp_semaphore_release(CSOUND *csound, sem_t *sem)
{
    TRACE_2("[%i] post:\n", csp_thread_index_get(csound));

    sem_post(sem);
}

void csp_semaphore_release_end(CSOUND *csound, sem_t *sem)
{
    int val, res;
    do {
      res = sem_getvalue(sem, &val);
      if (val<=0) sem_post(sem);
    } while (val==0);
}

void csp_semaphore_release_print(CSOUND *csound, sem_t *sem)
{
    int val, res;
    if (sem == NULL) csound->Die(csound, "Invalid NULL Parameter sem");
    do {
      res = sem_getvalue(sem, &val);
      if (val<=0) {
        TRACE_1("Release\n");
        sem_post(sem);
      }
    } while (val==0);
}



/***********************************************************************
 * set data structure
 */
#pragma mark -
#pragma mark Set

/* static prototypes */
static int set_element_delloc(CSOUND *csound, struct set_element_t **set_element);
static int set_element_alloc(CSOUND *csound,
                             struct set_element_t **set_element, char *data);
static int set_is_set(CSOUND *csound, struct set_t *set);
static int set_element_is_set_element(CSOUND *csound,
                                      struct set_element_t *set_element);

int csp_set_alloc(CSOUND *csound, struct set_t **set,
                  set_element_data_eq *ele_eq_func,
                  set_element_data_print *ele_print_func)
{
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");

    *set = csound->Malloc(csound, sizeof(struct set_t));
    if (*set == NULL) {
        csound->Die(csound, "Failed to allocate set");
    }
    memset(*set, 0, sizeof(struct set_t));
    strncpy((*set)->hdr, SET_HDR, HDR_LEN);
    (*set)->ele_eq_func = ele_eq_func;
    (*set)->ele_print_func = ele_print_func;
    (*set)->cache = NULL;

    return CSOUND_SUCCESS;
}

int csp_set_dealloc(CSOUND *csound, struct set_t **set)
{
    struct set_element_t *ele, *next = NULL;
    if (set == NULL || *set == NULL)
      csound->Die(csound, "Invalid NULL Parameter set");
    if (!set_is_set(csound, *set))
      csound->Die(csound, "Invalid Parameter set not a set");

    if ((*set)->cache != NULL) csound->Free(csound, (*set)->cache);

    ele = (*set)->head;
    while (ele != NULL) {
        next = ele->next;
        set_element_delloc(csound, &ele);
    }

    csound->Free(csound, *set);
    *set = NULL;

    return CSOUND_SUCCESS;
}

static int set_element_alloc(CSOUND *csound,
                             struct set_element_t **set_element, char *data)
{
    if (set_element == NULL || data == NULL)
      csound->Die(csound, "Invalid NULL Parameter data");

    *set_element = csound->Malloc(csound, sizeof(struct set_element_t));
    if (*set_element == NULL) {
        csound->Die(csound, "Failed to allocate set element");
    }
    memset(*set_element, 0, sizeof(struct set_element_t));
    strncpy((*set_element)->hdr, SET_ELEMENT_HDR, HDR_LEN);
    (*set_element)->data = data;

    return CSOUND_SUCCESS;
}

static int set_element_delloc(CSOUND *csound, struct set_element_t **set_element)
{
    if (set_element == NULL || *set_element == NULL)
      csound->Die(csound, "Invalid NULL Parameter set_element");
    csound->Free(csound, *set_element);
    *set_element = NULL;

    return CSOUND_SUCCESS;
}

static int set_is_set(CSOUND *csound, struct set_t *set)
{
    char buf[4];
    if (set == NULL) return 0;
    strncpy(buf, (char *)set, HDR_LEN);
    buf[3] = 0;
    return strcmp(buf, SET_HDR) == 0;
}

static int set_element_is_set_element(CSOUND *csound,
                                      struct set_element_t *set_element)
{
    if (set_element == NULL) return 0;
    char buf[4];
    strncpy(buf, (char *)set_element, HDR_LEN);
    buf[3] = 0;
    return strcmp(buf, SET_ELEMENT_HDR) == 0;
}

int csp_set_alloc_string(CSOUND *csound, struct set_t **set)
{
    return csp_set_alloc(csound, set,
                         csp_set_element_string_eq, csp_set_element_string_print);
}

int csp_set_element_string_eq(struct set_element_t *ele1,
                              struct set_element_t *ele2)
{
    return strcmp((char *)ele1->data, (char *)ele2->data) == 0;
}

int csp_set_element_ptr_eq(struct set_element_t *ele1, struct set_element_t *ele2)
{
    return (ele1->data == ele2->data);
}

void csp_set_element_string_print(CSOUND *csound, struct set_element_t *ele)
{
    csound->Message(csound, "%s", (char *)ele->data);
}

void csp_set_element_ptr_print(CSOUND *csound, struct set_element_t *ele)
{
    csound->Message(csound, "%p", ele->data);
}

static int set_update_cache(CSOUND *csound, struct set_t *set)
{
    if (set->cache != NULL) {
        csound->Free(csound, set->cache);
        set->cache = NULL;
    }
    if (set->count > 0) {
        set->cache = csound->Malloc(csound,
                                    sizeof(struct set_element_t *) * set->count);

        struct set_element_t *ele = set->head;
        int ctr = 0;
        while (ele != NULL) {
            set->cache[ctr] = ele;
            ctr++;
            ele = ele->next;
        }
    }
    return CSOUND_SUCCESS;
}

/*
 * if out_set_element is not NULL and the element corresponding to data is not found
 * it will not be changed
 */
static int set_element_get(CSOUND *csound, struct set_t *set,
                           char *data, struct set_element_t **out_set_element)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (data == NULL) csound->Die(csound, "Invalid NULL Parameter data");
    if (out_set_element == NULL)
      csound->Die(csound, "Invalid NULL Parameter out_set_element");
#endif

    struct set_element_t *ele = set->head;
    struct set_element_t data_ele = { SET_ELEMENT_HDR, data, 0 };
    while (ele != NULL) {
        if (set->ele_eq_func(ele, &data_ele)) {
            *out_set_element = ele;
            break;
        }
        ele = ele->next;
    }
    return CSOUND_SUCCESS;
}

int csp_set_add(CSOUND *csound, struct set_t *set, void *data)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (data == NULL) csound->Die(csound, "Invalid NULL Parameter data");
#endif

    if (csp_set_exists(csound, set, data)) {
        return CSOUND_SUCCESS;
    }

    struct set_element_t *ele = NULL;
    set_element_alloc(csound, &ele, data);
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

    return CSOUND_SUCCESS;
}

int csp_set_remove(CSOUND *csound, struct set_t *set, void *data)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (data == NULL) csound->Die(csound, "Invalid NULL Parameter data");
#endif

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

    set_update_cache(csound, set);

    return CSOUND_SUCCESS;
}

int csp_set_exists(CSOUND *csound, struct set_t *set, void *data)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (data == NULL) csound->Die(csound, "Invalid NULL Parameter data");
#endif

    struct set_element_t *ele = NULL;
    set_element_get(csound, set, data, &ele);

    return (ele == NULL ? 0 : 1);
}

int csp_set_print(CSOUND *csound, struct set_t *set)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (!set_is_set(csound, set))
      csound->Die(csound, "Invalid Parameter set not a set");
#endif

    struct set_element_t *ele = set->head;

    csound->Message(csound, "{ ");
    while (ele != NULL) {
        set->ele_print_func(csound, ele);

        TRACE_3(" [%p]", ele);

        if (ele->next != NULL) csound->Message(csound, ", ");
        ele = ele->next;
    }
    csound->Message(csound, " }\n");

    return CSOUND_SUCCESS;
}

int inline csp_set_count(CSOUND *csound, struct set_t *set)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (!set_is_set(csound, set))
      csound->Die(csound, "Invalid Parameter set not a set");
#endif

    return set->count;
}

/* 0 indexed */
int inline csp_set_get_num(CSOUND *csound, struct set_t *set, int num, void **data)
{
#ifdef SET_DEBUG
    if (set == NULL) csound->Die(csound, "Invalid NULL Parameter set");
    if (!set_is_set(csound, set))
      csound->Die(csound, "Invalid Parameter set not a set");
    if (num >= set->count)
      csound->Die(csound, "Invalid Parameter num is out of bounds");
    if (data == NULL) csound->Die(csound, "Invalid NULL Parameter data");
#endif

    *data = set->cache[num]->data;
    /*
    if (set->cache != NULL) {

    }
    else {
    int ctr = 0;
    struct set_element_t *ele = set->head;
    while (ctr < num && ele != NULL) {
    ctr++;
    ele = ele->next;
        }
        if (ctr == num && ele != NULL) {
            *data = ele->data;
        }
    }
    */
    return CSOUND_SUCCESS;
}

int csp_set_union(CSOUND *csound, struct set_t *first,
                  struct set_t *second, struct set_t **result)
{
#ifdef SET_DEBUG
    if (first == NULL) csound->Die(csound, "Invalid NULL Parameter first");
    if (!set_is_set(csound, first))
      csound->Die(csound, "Invalid Parameter set not a first");
    if (second == NULL) csound->Die(csound, "Invalid NULL Parameter second");
    if (!set_is_set(csound, second))
      csound->Die(csound, "Invalid Parameter set not a second");
    if (result == NULL) csound->Die(csound, "Invalid NULL Parameter result");
    if (first->ele_eq_func != second->ele_eq_func)
      csound->Die(csound, "Invalid sets for comparison (different equality)");
#endif

    csp_set_alloc(csound, result, first->ele_eq_func, first->ele_print_func);

    int ctr = 0;
    int first_len = csp_set_count(csound, first);
    int second_len = csp_set_count(csound, second);

    while (ctr < first_len) {
        void *data = NULL;
        csp_set_get_num(csound, first, ctr, &data);
        csp_set_add(csound, *result, data);
        ctr++;
    }

    ctr = 0;
    while (ctr < second_len) {
        void *data = NULL;
        csp_set_get_num(csound, second, ctr, &data);
        csp_set_add(csound, *result, data);
        ctr++;
    }

    return CSOUND_SUCCESS;
}

int csp_set_intersection(CSOUND *csound, struct set_t *first,
                         struct set_t *second, struct set_t **result)
{
#ifdef SET_DEBUG
    if (first == NULL) csound->Die(csound, "Invalid NULL Parameter first");
    if (!set_is_set(csound, first))
      csound->Die(csound, "Invalid Parameter set not a first");
    if (second == NULL) csound->Die(csound, "Invalid NULL Parameter second");
    if (!set_is_set(csound, second))
      csound->Die(csound, "Invalid Parameter set not a second");
    if (result == NULL) csound->Die(csound, "Invalid NULL Parameter result");
    if (first->ele_eq_func != second->ele_eq_func)
      csound->Die(csound, "Invalid sets for comparison (different equality)");
#endif

    csp_set_alloc(csound, result, first->ele_eq_func, first->ele_print_func);

    int ctr = 0;
    int first_len = csp_set_count(csound, first);

    while (ctr < first_len) {
        void *data = NULL;
        csp_set_get_num(csound, first, ctr, &data);
        if (csp_set_exists(csound, second, data)) {
            csp_set_add(csound, *result, data);
        }
        ctr++;
    }

    return CSOUND_SUCCESS;
}
