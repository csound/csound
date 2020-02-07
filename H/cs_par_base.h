/*
    cs_par_base.h:

    Copyright (C) 2011, 2017 John ffitch and Stephen Kyne

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

#ifndef __CS_PAR_BASE_H__
#define __CS_PAR_BASE_H__

// Semaphone.h only exists when using pthreads, doesn't apply to Windows
#ifndef WIN32
  #include <semaphore.h>
#endif

/* #define TAKE_LOCK(x) pthread_spin_lock(x)
#define RELS_LOCK(x) pthread_spin_unlock(x)
#define LOCK_TYPE  pthread_spinlock_t
#define INIT_LOCK(x)  pthread_spin_init(&(x), PTHREAD_PROCESS_PRIVATE)*/

/* #define TAKE_LOCK(x) pthread_mutex_lock(x) */
/* #define RELS_LOCK(x) pthread_mutex_unlock(x) */
/* #define LOCK_TYPE  pthread_mutex_t */
/* #define INIT_LOCK(x)  pthread_mutex_init(&(x), NULL) */

#if !defined(HAVE_PTHREAD_SPIN_LOCK)
// Windows environment should use native threads
# if WIN32
 #define TAKE_LOCK(x) csoundLockMutex(x)
 #define RELS_LOCK(x) csoundUnlockMutex(x)
 #define LOCK_TYPE  LPCRITICAL_SECTION
 // PTHREAD: FIXME no init function? unless createMutex should be used
 //          but has a different function signature
 #define INIT_LOCK(x) csoundCreateMutex(0)
# else
 /* VL: 18.05.2011 enabled this to allow OSX build */
 #define TAKE_LOCK(x) pthread_mutex_lock(x)
 #define RELS_LOCK(x) pthread_mutex_unlock(x)
 #define LOCK_TYPE  pthread_mutex_t
 #define INIT_LOCK(x)  pthread_mutex_init(&(x), NULL)
# endif
 #else
 #define TAKE_LOCK(x) pthread_spin_lock(x)
 #define RELS_LOCK(x) pthread_spin_unlock(x)
 #define LOCK_TYPE  pthread_spinlock_t
 #define INIT_LOCK(x)  pthread_spin_init(&(x), PTHREAD_PROCESS_PRIVATE)
#endif

#define DYNAMIC_2_SERIALIZE_PAR

/* #define TIMING */

/* #define SPINLOCK_BARRIER */
/* #define SPINLOCK_2_BARRIER */

#define HASH_CACHE
/* #define HYBRID_HASH_CACHE */
/* #define LINEAR_CACHE */

/* #define CACLULATE_WEIGHTS_BUILD */
#define LOOKUP_WEIGHTS

#define KPERF_SYM 0x31
#define BARRIER_1_WAIT_SYM 0x32
#define BARRIER_2_WAIT_SYM 0x33

/* return thread index of caller */
int csp_thread_index_get(CSOUND *csound);

/* structure headers */
#define HDR_LEN                 4
//#define INSTR_WEIGHT_INFO_HDR   "IWI"
#define INSTR_SEMANTICS_HDR     "SEM"
#define SET_ELEMENT_HDR         "STE"
#define SET_HDR                 "SET"
//#define DAG_2_HDR               "DG2"
//#define DAG_NODE_2_HDR          "DN2"
//#define SEMAPHORE_HDR           "SPS"
#define GLOBAL_VAR_LOCK_HDR     "GVL"
//#define SERIALIZED_DAG_HDR      "SDG"

/*
 * set structures
 *
 * set maintains insertion order of elements
 * implemented as a singly linked list
 */
struct set_element_t {
    char                 hdr[4];
    void                 *data;
    struct set_element_t *next;
};

struct set_t {
    char                  hdr[4];
    struct set_element_t *head;
    struct set_element_t *tail;
    int                  count;
    int     (*ele_eq_func)(struct set_element_t *, struct set_element_t *);
    void    (*ele_print_func)(CSOUND *, struct set_element_t *);
    struct set_element_t **cache;
};

/* function pointer types for set member equality */
typedef int (set_element_data_eq)(struct set_element_t *, struct set_element_t *);
int csp_set_element_string_eq(struct set_element_t *ele1,
                              struct set_element_t *ele2);
int csp_set_element_ptr_eq(struct set_element_t *ele1,
                           struct set_element_t *ele2);

/* function pointer types for set member printing */
typedef void (set_element_data_print)(CSOUND *, struct set_element_t *);
void csp_set_element_string_print(CSOUND *csound, struct set_element_t *ele);
void csp_set_element_ptr_print(CSOUND *csound, struct set_element_t *ele);

/* allocating sets with specification of element equality and printing functions */
struct set_t *csp_set_alloc(CSOUND *csound,
                            set_element_data_eq *ele_eq_func,
                            set_element_data_print *ele_print_func);
void csp_set_dealloc(CSOUND *csound, struct set_t **set);
/* shortcut to get a set of strings uses string element equality and
   printing functions */
struct set_t *csp_set_alloc_string(CSOUND *csound);

/* functions to manipulate set, return CSOUND_SUCCESS if successful */
void csp_set_add(CSOUND *csound,     struct set_t *set, void *data);
void csp_set_remove(CSOUND *csound,  struct set_t *set, void *data);
/* check element existance returns 1 if data exists */
void csp_set_print(CSOUND *csound, struct set_t *set);

/* get a count and access members */
int csp_set_count(struct set_t *set);

/*
 * set union and intersection
 * allocates a new set in result
 * union/intersect first and second putting into result
 */
struct set_t *csp_set_union(CSOUND *csound, struct set_t *first,
                   struct set_t *second);
struct set_t *csp_set_intersection(CSOUND *csound, struct set_t *first,
                          struct set_t *second);

/* spinlock */

/* semaphore */
/* struct semaphore_spin_t { */
/*     char    hdr[HDR_LEN]; */
/*     int     thread_count; */
/*     int     max_threads; */
/*     int     arrived; */
/*     int     held; */
/*     int     spinlock; */
/*     int     count; */
/*     int     lock; */
/*     int     *key; */
/*     int     locks[]; */
/* }; */

// Kludge to allow us to pass in HANDLE objects to be used as semaphore whilst
// supporting the traditional pthread way for non Windows platforms
// FIXME, does this even work? API's take ** versions of sem_t
#ifdef WIN32
typedef HANDLE sem_t;
#endif

/* create a semaphore with a maximum number of threads
 * initially 1 thread is allowed in
 */
void csp_semaphore_alloc(CSOUND *csound, sem_t **sem,
                         int max_threads);
void csp_semaphore_dealloc(CSOUND *csound, sem_t **sem);
/* wait at the semaphore. if the number allowed in is greater than the
 * number arrived calling thread continues
 * otherwise thread blocks until semaphore is grown
 */
void csp_semaphore_wait(CSOUND *csound, sem_t *sem);
/* increase the number of threads allowed in by 1 */
void csp_semaphore_grow(CSOUND *csound, sem_t *sem);
/* reduce the number of threads allowed in and the arrive count by 1
 * call this when calling thread is finished with the semaphore. */
void csp_semaphore_release(CSOUND *csound, sem_t *sem);
/* call when all threads are done with the resource the semaphore is protecting.
 * releases all blocked threads. */
void csp_semaphore_release_end(CSOUND *csound, sem_t *sem);
/* print semaphore info */
void csp_semaphore_release_print(CSOUND *csound, sem_t *sem);

#endif /* end of include guard: __CS_PAR_BASE_H__ */
