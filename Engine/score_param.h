#ifndef __SCORE_PARAM_H
#define __SCORE_PARAM_H


typedef struct ListItem {
  double           val;
  struct ListItem *args;
} ListItem;

typedef struct ScoreTree {
  int              op;
  ListItem         *args;
  struct ScoreTree *next;
  int              line;
  int              locn;
} ScoreTree;

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
    CORFIL          *cf;
    MACRO           *macros;
    MACRON          *alt_stack; //[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    unsigned int macro_stack_size;
    IFDEFSTACK      *ifdefStack;
    unsigned char   isIfndef;
    unsigned char   isString;
    uint16_t        line;
    uint32_t        locn;
    uint32_t        llocn;
    uint16_t        depth;
    uint16_t        lstack[1024];
         /* Variable for nested repeat loops */
#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */
    char    repeat_name_n[RPTDEPTH][NAMELEN];
    int     repeat_cnt_n[RPTDEPTH];
    int     repeat_indx[RPTDEPTH];
    CORFIL  *cf_stack[RPTDEPTH];
    int     repeat_inc_n /* = 1 */;
    MACRO   *repeat_mm_n[RPTDEPTH];
    int     repeat_index;
         /* Variables for section repeat */
    int     in_repeat_sect;
    int     repeat_sect_cnt;
    int     repeat_sect_index;
    int     repeat_sect_line;
    CORFIL  *repeat_sect_cf;
    MACRO   *repeat_sect_mm;
} PRS_PARM;

typedef struct scotoken_s {
    int             type;
    int             ival;
    MYFLT           fval;
    char            *strbuff;
} SCOTOKEN;

typedef struct score_parm_s {
    void            *yyscanner;
    int             locn;
    MACRO           *macros;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
    int             ival;
    MYFLT           fval;
    SCOTOKEN        *arglist;
} SCORE_PARM;

uint32_t make_location(PRS_PARM *);
extern uint8_t file_to_int(CSOUND*, const char*);

#endif
