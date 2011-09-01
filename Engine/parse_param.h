#define MARGS   (3)
#define MAX_INCLUDE_DEPTH 100
struct MACRO;

typedef struct MACRON {
  int      n;
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

typedef struct parse_parm_s {
    void            *yyscanner;
    char            *buffer;
//  int             pos;
//  int             length;
//  double          result;
    MACRO           *macros;
//  unsigned int    macro_stack_ptr;
    int             nBuffer;
    int             lBuffer;
    MACRON alt_stack[MAX_INCLUDE_DEPTH];
    unsigned int macro_stack_ptr;
    char            *xstrbuff;
    int             xstrptr,xstrmax;
} PARSE_PARM;

#define lMaxBuffer (1000)
void    cs_init_math_constants_macros(CSOUND*, void*);
void    cs_init_omacros(CSOUND*, void*, NAMES*);

