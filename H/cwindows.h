/*******************************************************\
*       window.h                                        *
*       portable window graphs stolen from Csound       *
*       necessary header declarations                   *
*       08nov90 dpwe                                    *
\*******************************************************/

#ifndef NULL
#define NULL 0L
#endif


#define CAPSIZE  60

typedef struct {
        long    windid;                 /* set by MakeGraph() */
        MYFLT   *fdata;                 /* data passed to DrawGraph */
        long    npts;                   /* size of above array */
        char    caption[CAPSIZE];       /* caption string for graph */
        short   waitflg;                /* set =1 to wait for ms after Draw */
        short   polarity;               /* controls positioning of X axis */
        MYFLT   max, min;               /* workspace .. extrema this frame */
        MYFLT   absmax;                 /* workspace .. largest of above */
        MYFLT   oabsmax;                /* Y axis scaling factor */
        int     danflag;                /* set to 1 for extra Yaxis mid span */
} WINDAT;

enum {        /* symbols for WINDAT.polarity field */
        NOPOL,
        NEGPOL,
        POSPOL,
        BIPOL
};

typedef struct {        /* for 'joystick' input window */
        long     windid;        /* xwindow handle */
        int      m_x,m_y;       /* current crosshair pixel adr */
        MYFLT    x,y;           /* current proportions of fsd */
        int      down;
} XYINDAT;

/* WINDAT *SetDisp(); */
void dispset(WINDAT *, MYFLT *, long, char *, int, char *), display(WINDAT *);
void dispinit(void);
int dispexit(void);
void display(WINDAT*);
WINDAT *NewWin(char *, int);
void   DoDisp(WINDAT *,MYFLT *,int);

int  Graphable(void);           /* initialise windows.  Returns 1 if X ok */
void MakeGraph(WINDAT *, char *);       /* create wdw for a graph */
void MakeXYin(XYINDAT *, MYFLT, MYFLT);
                                /* create a mouse input window; init scale */
void DrawGraph(WINDAT *);       /* update graph in existing window */
void ReadXYin(XYINDAT *);       /* fetch latest value from ms input wdw */
void KillGraph(WINDAT *);       /* remove a graph window */
void KillXYin(XYINDAT *);       /* remove a ms input window */
int  ExitGraph(void); /* print click-Exit message in most recently active window */

