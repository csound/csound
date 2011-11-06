#ifndef __CS_PAR_BASE_H__
#define __CS_PAR_BASE_H__

#include <semaphore.h>

/* #define TAKE_LOCK(x) pthread_spin_lock(x)
#define RELS_LOCK(x) pthread_spin_unlock(x)
#define LOCK_TYPE  pthread_spinlock_t
#define INIT_LOCK(x)  pthread_spin_init(&(x), PTHREAD_PROCESS_PRIVATE)*/

/* #define TAKE_LOCK(x) pthread_mutex_lock(x) */
/* #define RELS_LOCK(x) pthread_mutex_unlock(x) */
/* #define LOCK_TYPE  pthread_mutex_t */
/* #define INIT_LOCK(x)  pthread_mutex_init(&(x), NULL) */

#if !defined(HAVE_PTHREAD_SPIN_LOCK) /* VL: 18.05.2011 enabled this to allow OSX build */
 # define TAKE_LOCK(x) pthread_mutex_lock(x) 
 # define RELS_LOCK(x) pthread_mutex_unlock(x) 
 # define LOCK_TYPE  pthread_mutex_t 
 # define INIT_LOCK(x)  pthread_mutex_init(&(x), NULL) 

 #else 
 # define TAKE_LOCK(x) pthread_spin_lock(x) 
 # define RELS_LOCK(x) pthread_spin_unlock(x) 
 # define LOCK_TYPE  pthread_spinlock_t 
 # define INIT_LOCK(x)  pthread_spin_init(&(x), PTHREAD_PROCESS_PRIVATE) 
#endif 

#define DYNAMIC_2_SERIALIZE_PAR

#define TRACE 0
/* #define TIMING */

/* #define SPINLOCK_BARRIER */
/* #define SPINLOCK_2_BARRIER */

#define HASH_CACHE
/* #define HYBRID_HASH_CACHE */
/* #define LINEAR_CACHE */

/* #define CACLULATE_WEIGHTS_BUILD */
#define LOOKUP_WEIGHTS

#ifdef TIMING
  #define TIMER_INIT(val, name) RTCLOCK val ## _timer;
  #define TIMER_START(val, name) \
              csound->InitTimerStruct(& val ## _timer); \
              csound->Message(csound, name "Start: %f\n", \
                              csound->GetRealTime(& val ## _timer));
  #define TIMER_END(val, name) \
              csound->Message(csound, name "End: %f\n", \
                              csound->GetRealTime(& val ## _timer));

  #define TIMER_T_START(val, index, name) \
              csound->InitTimerStruct(& val ## _timer); \
              csound->Message(csound, "[%i] " name "Start: %f\n", \
                              index, csound->GetRealTime(& val ## _timer));
  #define TIMER_T_END(val, index, name) \
              csound->Message(csound, "[%i] " name "End: %f\n", \
                              index, csound->GetRealTime(& val ## _timer));
#else
  #define TIMER_INIT(val, name)
  #define TIMER_START(val, name)
  #define TIMER_END(val, name)
  #define TIMER_T_START(val, index, name)
  #define TIMER_T_END(val, index, name)
#endif

#if (TRACE&1) == 1
#define TRACE_0(...) { csound->Message(csound,"0:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_0(...)
#endif
#if (TRACE&2) == 2
#define TRACE_1(...) { csound->Message(csound,"1:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_1(...)
#endif
#if (TRACE&4) == 4
    #define TRACE_2(...) { csound->Message(csound,"2:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_2(...)
#endif
#if (TRACE&8) == 8
    #define TRACE_3(...) { csound->Message(csound,"3:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_3(...)
#endif
#if (TRACE&16) == 16
    #define TRACE_4(...) { csound->Message(csound,"4:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_4(...)
#endif
#if (TRACE&32) == 32
    #define TRACE_5(...) { csound->Message(csound,"5:"); csound->Message(csound, __VA_ARGS__);}
#else
    #define TRACE_5(...)
#endif

#define KPERF_SYM 0x31
#define BARRIER_1_WAIT_SYM 0x32
#define BARRIER_2_WAIT_SYM 0x33
#define SHARK_SIGNPOST(sym)

/* return thread index of caller */
int csp_thread_index_get(CSOUND *csound);

/* structure headers */
#define HDR_LEN                 4
#define INSTR_WEIGHT_INFO_HDR   "IWI"
#define INSTR_SEMANTICS_HDR     "SEM"
#define SET_ELEMENT_HDR         "STE"
#define SET_HDR                 "SET"
#define DAG_2_HDR               "DG2"
#define DAG_NODE_2_HDR          "DN2"
#define SEMAPHORE_HDR           "SPS"
#define GLOBAL_VAR_LOCK_HDR     "GVL"
#define SERIALIZED_DAG_HDR      "SDG"

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
int csp_set_alloc(CSOUND *csound,   struct set_t **set,
                  set_element_data_eq *ele_eq_func,
                  set_element_data_print *ele_print_func);
int csp_set_dealloc(CSOUND *csound, struct set_t **set);
/* shortcut to get a set of strings uses string element equality and
   printing functions */
int csp_set_alloc_string(CSOUND *csound, struct set_t **set);

/* functions to manipulate set, return CSOUND_SUCCESS if successful */
int csp_set_add(CSOUND *csound,     struct set_t *set, void *data);
int csp_set_remove(CSOUND *csound,  struct set_t *set, void *data);
/* check element existance returns 1 if data exists */
int csp_set_exists(CSOUND *csound,  struct set_t *set, void *data);
int csp_set_print(CSOUND *csound, struct set_t *set);

/* get a count and access members */
extern int inline csp_set_count(CSOUND *csound, struct set_t *set);
extern int inline csp_set_get_num(CSOUND *csound, struct set_t *set, int num, void **data);

/* 
 * set union and intersection
 * allocates a new set in result
 * union/intersect first and second putting into result
 */
int csp_set_union(CSOUND *csound, struct set_t *first,
                  struct set_t *second, struct set_t **result);
int csp_set_intersection(CSOUND *csound, struct set_t *first,
                         struct set_t *second, struct set_t **result);

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
