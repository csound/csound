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
//  MACRO           *macros;
//  unsigned int    macro_stack_ptr;
} PARSE_PARM;

#define lMaxBuffer (1000)
