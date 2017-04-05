#ifndef __PARSE_PARAM_H
#define __PARSE_PARAM_H

#define MARGS   (3)
#define MAX_INCLUDE_DEPTH 100
struct MACRO;

typedef struct MACRON {
  int             n;
  unsigned int    line;
  struct MACRO    *s;
} MACRON;

typedef struct MACRO {          /* To store active macros */
    char          *name;        /* Use is by name */
    int           acnt;         /* Count of arguments */
    char          *body;        /* The text of the macro */
    struct MACRO  *next;        /* Chain of active macros */
    int           margs;        /* amount of space for args */
    char          *arg[MARGS];  /* With these arguments */
} MACRO;

typedef struct IFDEFSTACK_ {
    struct IFDEFSTACK_  *prv;
    unsigned char   isDef;      /* non-zero if #ifdef is true, or #ifndef   */
                                /*   is false                               */
    unsigned char   isElse;     /* non-zero between #else and #endif        */
    unsigned char   isSkip;     /* sum of: 1: skipping code due to this     */
                                /*   #ifdef, 2: skipping due to parent      */
} IFDEFSTACK;


typedef struct pre_parm_s {
    void            *yyscanner;
    MACRO           *macros;
    MACRON          *alt_stack; //[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    unsigned int macro_stack_size;
    IFDEFSTACK      *ifdefStack;
    unsigned char   isIfndef;
    unsigned char   isString;
    uint16_t        line;
    uint64_t        locn;
    uint64_t        llocn;
    uint16_t        depth;
    uint16_t        lstack[1024];
} PRE_PARM;

typedef struct parse_parm_s {
    void            *yyscanner;
    uint64_t        locn;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
    uint64_t        iline;      /* Line number for start of instrument */
    uint64_t        ilocn;      /* and location */
} PARSE_PARM;

void    cs_init_math_constants_macros(CSOUND*, PRE_PARM*);
void    cs_init_omacros(CSOUND*, PRE_PARM*, NAMES*);

uint64_t make_location(PRE_PARM *);
extern uint8_t file_to_int(CSOUND*, const char*);

extern void csound_orcput_ilocn(void *, uint64_t, uint64_t);
extern uint64_t csound_orcget_iline(void *);
extern uint64_t csound_orcget_ilocn(void *);
#endif
