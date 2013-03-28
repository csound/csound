#ifndef __SCORE_PARAM_H
#define __SCORE_PARAM_H

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


typedef struct prs_parm_s {
    void            *yyscanner;
    MACRO           *macros;
    MACRON alt_stack[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    IFDEFSTACK      *ifdefStack;
    unsigned char   isIfndef;
    unsigned char   isString;
    uint16_t        line;
    uint32_t        locn;
    uint32_t        llocn;
    uint16_t        depth;
    uint8_t         lstack[1024];
} PRS_PARM;

typedef struct score_parm_s {
    void            *yyscanner;
    int             locn;
    MACRO           *macros;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
    int             ival;
    MYFLT           fval;
} SCORE_PARM;

uint32_t make_location(PRS_PARM *);
extern uint8_t file_to_int(CSOUND*, const char*);

typedef struct scotoken_s {
    int             type;
    int             ival;
    MYFLT           fval;
    char            *strbuff;
} SCOTOKEN;
#endif
