/*
    cscorfns.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"     /*                      CSCORFNS.C      */
#include "cscore.h"
#include "corfile.h"

#define TYP_FREE   0
#define TYP_EVENT  1
#define TYP_EVLIST 2
#define TYP_SPACE  3
#define NSLOTS     100      /* default slots in cscoreListCreate list   */
#define MAXALLOC   32768L

extern int lplay(CSOUND *, EVLIST *);

typedef struct space {
        CSHDR  h;
        struct space *nxtspace;
} SPACE;

/* RWD: moved from below for use in reset */
typedef struct {
        FILE  *iscfp;
        EVENT *next;
        MYFLT until;
        int   wasend, warped, atEOF;
} INFILE;
static INFILE *infiles = NULL;    /* array of infile status blks */

/* next two from cscoreDefineEvent() really */
static  EVENT  *evtmp = NULL;
static  EVTBLK *evtmpblk;

static SPACE  spaceanchor = { { NULL, NULL, TYP_SPACE, 0 }, NULL };
static CSHDR  *nxtfree = NULL;   /* fast pointer to yet unused free space */
static EVENT  *nxtevt = NULL;    /* to hold nxt infil event, PMAX pfields */
static EVTBLK *nxtevtblk;        /* cs.h EVTBLK subset of EVENT nxtevt    */
static int    warpout = 0;
static MYFLT  curuntil;          /* initialised to zero by cscoreFileOpen */
static int    wasend;            /* ditto */
static int    atEOF;             /* zero when file opened;
                                    stays one once rdscor returns 0 */


void cscoreRESET(CSOUND *csound)
{
    nxtfree   = NULL;
    nxtevt    = NULL;
    nxtevtblk = NULL;
    infiles   = NULL;            /* FIXME: leak? (any others here?) */
    csound->warped  = warpout  = 0;
    evtmp     = NULL;
    evtmpblk  = NULL;
    if (spaceanchor.nxtspace != NULL) {
      SPACE *p = spaceanchor.nxtspace;
      SPACE *n;
      do {
        n = p->nxtspace;
        csound->Free(csound, p);
        p = n;
      }
      while (p != NULL);
    }
    spaceanchor.nxtspace = NULL;
    spaceanchor.h.prvblk = NULL;
    spaceanchor.h.nxtblk = NULL;
    spaceanchor.h.type = TYP_SPACE;
    spaceanchor.h.size = 0;
    return;
}

static SPACE *morespace(CSOUND *csound)
{                               /* alloc large amount of memory, keep in a */
    SPACE *space, *prvspace;    /* chain. Put SPACE blk at top & init rem as */
    CSHDR *free;                /* a FREE blk */

    prvspace = &spaceanchor;
    while ((space = prvspace->nxtspace) != NULL)
      prvspace = space;
    space = (SPACE *) csound->Malloc(csound, (long) MAXALLOC);
    prvspace->nxtspace = space;
    space->nxtspace = NULL;
    space->h.prvblk = NULL;
    space->h.nxtblk = (CSHDR *) ((char *) space + sizeof(SPACE));
    space->h.type = TYP_SPACE;
    space->h.size = sizeof(SPACE);
    free = space->h.nxtblk;
    free->prvblk = (CSHDR *) space;    /* init rem as a TYP_FREE blk */
    free->nxtblk = NULL;
    free->type = TYP_FREE;
    free->size = MAXALLOC - sizeof(SPACE);
    return(space);
}

/* search space chains for min size free blk */
/* else alloc new space blk & reset fast free */

static CSHDR *getfree(CSOUND *csound, int minfreesiz)
{
    SPACE *curspace;
    CSHDR *blkp;

    curspace = &spaceanchor;
    while ((curspace = curspace->nxtspace) != NULL) {
      blkp = curspace->h.nxtblk;
      do {
        if (blkp->type == TYP_FREE && blkp->size >= minfreesiz)
          return(blkp);
      } while ((blkp = blkp->nxtblk) != NULL);
    }
    curspace = morespace(csound);        /* else alloc more space, and  */
    nxtfree = curspace->h.nxtblk;        /* reset the fast free pointer */
    return(nxtfree);
}

/* return a TYP_EVENT or TYP_EVLIST to free space */
/* consolidate with any prev or follow free space */

static void csfree(CSHDR *bp)
{
    CSHDR *prvp, *nxtp;

    if ((prvp = bp->prvblk) != NULL && prvp->type == TYP_FREE) {
      if ((nxtp = bp->nxtblk) != NULL && nxtp->type == TYP_FREE) {
        if ((prvp->nxtblk = nxtp->nxtblk) != NULL)
          nxtp->nxtblk->prvblk = prvp;
        prvp->size += bp->size + nxtp->size;
      }
      else {
        if ((prvp->nxtblk = bp->nxtblk) != NULL)
          bp->nxtblk->prvblk = prvp;
        prvp->size += bp->size;
      }
    }
    else {
      if ((nxtp = bp->nxtblk) != NULL && nxtp->type == TYP_FREE) {
        if ((bp->nxtblk = nxtp->nxtblk) != NULL)
          nxtp->nxtblk->prvblk = bp;
        bp->size += nxtp->size;
      }
      bp->type = TYP_FREE;
    }
}

/* create an array of event pointer slots */

PUBLIC EVLIST * cscoreListCreate(CSOUND *csound, int nslots)
{
    CSHDR *newblk, *newfree;
    EVLIST *a;
    int   needsiz = sizeof(EVLIST) + nslots * sizeof(EVENT *);
    int   minfreesiz = needsiz + sizeof(CSHDR);

    if (UNLIKELY(minfreesiz > MAXALLOC)) {
      csound->Message(csound, Str("Not enough memory\n"));
      exit(1);
    }
    if (nxtfree != NULL && nxtfree->size >= minfreesiz)
      newblk = nxtfree;
    else newblk = getfree(csound, minfreesiz);
    newfree = (CSHDR *) ((char *)newblk + needsiz);
    newfree->prvblk = newblk;
    newfree->nxtblk = newblk->nxtblk;
    newfree->type = TYP_FREE;
    newfree->size = newblk->size - needsiz;
    newblk->nxtblk = newfree;
    newblk->type = TYP_EVLIST;
    newblk->size = needsiz;
    if (newblk == nxtfree)  nxtfree = newfree;
    a = (EVLIST *) newblk;
    a->nslots = nslots;
    a->nevents= 0;
    return(a);
}

/* creat a new event space */

PUBLIC EVENT * cscoreCreateEvent(CSOUND *csound, int pcnt)
{
    CSHDR *newblk, *newfree;
    EVENT *e;
    int   needsiz = sizeof(EVENT) + pcnt * sizeof(MYFLT);
    int   minfreesiz = needsiz + sizeof(CSHDR);

    if (UNLIKELY(minfreesiz > MAXALLOC)) {
      csound->Message(csound, Str("Not enough memory\n"));
      exit(1);
    }
    if (nxtfree != NULL && nxtfree->size >= minfreesiz)
      newblk = nxtfree;
    else newblk = getfree(csound, minfreesiz);
    newfree = (CSHDR *) ((char *)newblk + needsiz);
    newfree->prvblk = newblk;
    newfree->nxtblk = newblk->nxtblk;
    newfree->type = TYP_FREE;
    newfree->size = newblk->size - needsiz;
    newblk->nxtblk = newfree;
    newblk->type = TYP_EVENT;
    newblk->size = needsiz;
    if (newblk == nxtfree)  nxtfree = newfree;
    e = (EVENT *) newblk;
    e->pcnt = pcnt;
    return(e);
}

/* make a new copy of an event */

PUBLIC EVENT * cscoreCopyEvent(CSOUND *csound, EVENT *e)
{
    EVENT *f;
    int  n;
    MYFLT *p, *q;

    n = e->pcnt;
    f = cscoreCreateEvent(csound, n);
    f->op = e->op;
    f->strarg = e->strarg;
    /* f->pcnt was set by cscoreCreateEvent */
    /* previous method of copying these was dangerous! - akozar */
    f->p2orig = e->p2orig;
    f->p3orig = e->p3orig;
    p = &e->p[0];
    q = &f->p[0];
    n += 1; /* p[] is one larger than pcnt b/c p[0] unused */
    while (n--)
      *q++ = *p++;
    return(f);
}

/* RWD: cannot addd init arg as this is a std public func */
/* Can only do reentry by moving statics outside: fortunately, */
/* names are unique */

/* define an event from string arg */

PUBLIC EVENT * cscoreDefineEvent(CSOUND *csound, char *s)
{
    MYFLT *p, *q;

    if (evtmp == NULL) {
      evtmp = cscoreCreateEvent(csound, PMAX);
      evtmpblk = (EVTBLK *) &evtmp->strarg;
    }
    while (*s == ' ')
      s++;
    evtmp->op = *s++;                       /* read opcode */
    while (*s == ' ')
      s++;
    p = &evtmp->p[1];
    q = &evtmp->p[PMAX];
#ifdef USE_DOUBLE
    while (CS_SSCANF(s,"%lf",p++) > 0)         /* read pfields */
#else
    while (CS_SSCANF(s,"%f",p++) > 0)          /* read pfields */
#endif
    {
      while ((*s >= '0' && *s <= '9') || *s == '.' || *s == '-')
        s++;
      while (*s == ' ')
        s++;
      if (UNLIKELY(p > q && *s != '\0'))  {           /* too many ? */
        p++;
        csound->Message(csound,
                        Str("PMAX exceeded, string event truncated.\n"));
        break;
      }
    }
    evtmp->pcnt = p - &evtmp->p[1] - 1;     /* set count of params recvd */
    evtmp->p2orig = evtmp->p[2];
    evtmp->p3orig = evtmp->p[3];
    return (cscoreCopyEvent(csound, evtmp));    /* copy event to a new space */
}

/* get nxt event from input score buf */
/*   and  refill the buf */

PUBLIC EVENT * cscoreGetEvent(CSOUND *csound)
{
    EVENT *e;

    if (!atEOF && nxtevt->op != '\0')
      e = cscoreCopyEvent(csound, nxtevt);
    else e = NULL;
    if (!(rdscor(csound, nxtevtblk))) {
      nxtevt->op = '\0';
      atEOF = 1;
    }
    return(e);
}

/* put an event to cscore outfile */

PUBLIC void cscorePutEvent(CSOUND *csound, EVENT *e)
{
    int  pcnt;
    MYFLT *q;
    int  c = e->op;

    if (c == 's')  warpout = 0;         /* new section:  init to non-warped */
    putc(c, csound->oscfp);
    q = &e->p[1];
    if ((pcnt = e->pcnt)) {
      if (pcnt--)       fprintf(csound->oscfp," %g",*q++);
      //else goto termin; /* cannot happen */
      if (pcnt--) {
        if (warpout) {    fprintf(csound->oscfp," %g", e->p2orig);}
                        fprintf(csound->oscfp," %g",*q++);
      }
      else goto termin;
      if (pcnt--) {
        if (warpout)  {  fprintf(csound->oscfp," %g", e->p3orig); }
                        fprintf(csound->oscfp," %g",*q++);
      }
      else goto termin;
      while (pcnt--)
        { fprintf(csound->oscfp," %g",*q++); }
    }
 termin:
    putc((int)'\n', csound->oscfp);
    if (c == 'w')  warpout = 1; /* was warp statement: sect now warped */
}

PUBLIC void cscorePutString(CSOUND *csound, char *s)
{
    fprintf(csound->oscfp,"%s\n", s);
    if (*s == 's')  warpout = 0;
    else if (*s == 'w') warpout = 1;
}

/* expand an event list by NSLOTS more slots */
/* copy the previous list, free up the old   */

static EVLIST * lexpand(CSOUND *csound, EVLIST *a)
{
    EVLIST *b;
    EVENT **p, **q;
    int n;

    b = cscoreListCreate(csound, a->nslots + NSLOTS);
    b->nevents = n = a->nevents;
    p = &a->e[1];
    q = &b->e[1];
    while (n--)
      *q++ = *p++;
    csfree((CSHDR *) a);
    return(b);
}

/* append an event to a list */

PUBLIC EVLIST * cscoreListAppendEvent(CSOUND *csound, EVLIST *a, EVENT *e)
{
    int  n;

    if ((n = a->nevents) == a->nslots)
      a = lexpand(csound, a);
    a->e[n+1] = e;
    a->nevents++;
    return(a);
}

/* append a string event to a list */

PUBLIC EVLIST * cscoreListAppendStringEvent(CSOUND *csound, EVLIST *a, char *s)
{
    EVENT *e = cscoreDefineEvent(csound, s);
    return(cscoreListAppendEvent(csound,a,e));
}

/* get section events from the scorefile */

PUBLIC EVLIST * cscoreListGetSection(CSOUND *csound)
{
    EVLIST *a;
    EVENT *e, **p;
    int nevents = 0;

    a = cscoreListCreate(csound, NSLOTS);
    p = &a->e[1];
    if (UNLIKELY(csound->scstr == NULL || csound->scstr->body[0] == '\0'))
      return a;
    while ((e = cscoreGetEvent(csound)) != NULL) {
      if (e->op == 's' || e->op == 'e')
        break;
      if (nevents == a->nslots) {
        a->nevents = nevents;
        a = lexpand(csound, a);
        p = &a->e[nevents+1];
      }
      *p++ = e;
      nevents++;
    }
    a->nevents = nevents;
    return(a);
}

/* get section events from the scorefile */

PUBLIC EVLIST * cscoreListGetUntil(CSOUND *csound, MYFLT beatno)
{
    EVLIST *a;
    EVENT *e, **p;
    int nevents = 0;
    char op;

    a = cscoreListCreate(csound, NSLOTS);
    p = &a->e[1];
    while ((op = nxtevt->op) == 't' || op == 'w' || op == 's' || op == 'e'
           || (op != '\0' && nxtevt->p2orig < beatno)) {
      e = cscoreGetEvent(csound);
      if (e->op == 's') {
        wasend = 1;
        break;
      }
      if (e->op == 'e')
        break;
      if (nevents == a->nslots) {
        a->nevents = nevents;
        a = lexpand(csound,a);
        p = &a->e[nevents+1];
      }
      *p++ = e;
      nevents++;
    }
    a->nevents = nevents;
    return(a);
}

/* get section events from the scorefile */

PUBLIC EVLIST * cscoreListGetNext(CSOUND *csound, MYFLT nbeats)
{
    if (wasend) {
      wasend = 0;
      curuntil = nbeats;
    }
    else curuntil += nbeats;
    return(cscoreListGetUntil(csound,curuntil));
}

/* put listed events to cscore output */

PUBLIC void cscoreListPut(CSOUND *csound, EVLIST *a)
{
    EVENT **p;
    int  n;

    n = a->nevents;
    p = &a->e[1];
    while (n--)
      cscorePutEvent(csound, *p++);
}

PUBLIC int cscoreListPlay(CSOUND *csound, EVLIST *a)
{
    return lplay(csound, a);
}

PUBLIC EVLIST * cscoreListCopy(CSOUND *csound, EVLIST *a)
{
    EVLIST *b;
    EVENT **p, **q;
    int  n = a->nevents;

    b = cscoreListCreate(csound, n);
    b->nevents = n;
    p = &a->e[1];
    q = &b->e[1];
    while (n--)
      *q++ = *p++;
    return(b);
}

PUBLIC EVLIST * cscoreListCopyEvents(CSOUND *csound, EVLIST *a)
{
    EVLIST *b;
    EVENT **p, **q;
    int  n = a->nevents;

    b = cscoreListCreate(csound, n);
    b->nevents = n;
    p = &a->e[1];
    q = &b->e[1];
    while (n--)
      *q++ = cscoreCopyEvent(csound, *p++);
    return(b);
}

PUBLIC EVLIST * cscoreListAppendList(CSOUND *csound, EVLIST *a, EVLIST *b)
{
    EVENT **p, **q;
    int i, j;

    i = a->nevents;
    j = b->nevents;
    if (i + j >= a->nslots) {
      EVLIST *c;
      int n = i;
      c = cscoreListCreate(csound, i+j);
      p = &a->e[1];
      q = &c->e[1];
      while (n--)
        *q++ = *p++;
      csfree((CSHDR *) a);
      a = c;
    }
    a->nevents = i+j;
    p = &a->e[i+1];
    q = &b->e[1];
    while (j--)
      *p++ = *q++;
    return(a);
}

PUBLIC EVLIST * cscoreListConcatenate(CSOUND *csound, EVLIST *a, EVLIST *b)
{
    return cscoreListAppendList(csound, a, b);
}

/* put evlist pointers into chronological order */

PUBLIC void cscoreListSort(CSOUND *csound, EVLIST *a)
{
    IGN(csound);
    EVENT **p, **q;
    EVENT *e, *f;
    int  n, gap, i, j;

    n = a->nevents;
    e = a->e[n];
    if (e->op == 's' || e->op == 'e')
      --n;
    for (gap = n/2;  gap > 0;  gap /=2)
      for (i = gap;  i < n;  i++)
        for (j = i-gap;  j >= 0;  j -= gap) {
          p = &a->e[j+1];     e = *p;
          q = &a->e[j+1+gap]; f = *q;
          if (e->op == 'w')
            break;
          if (e->p[2] < f->p[2])
            break;
          if (e->p[2] == f->p[2]) {
            if (e->op == f->op) {
              if (e->op == 'f')
                break;
              if (e->p[1] < f->p[1])
                break;
              if (e->p[1] == f->p[1])
                if (e->p[3] <= f->p[3])
                  break;
            }
            else if (e->op < f->op)
              break;
          }
          *p = f;  *q = e;
        }
}

PUBLIC EVLIST * cscoreListExtractInstruments(CSOUND *csound,
               EVLIST *a, char *s) /* list extract by instr numbers */
{
    int     x[5], xcnt;
    int     xn, *xp, insno, n;
    EVENT   **p, **q, *e;
    EVLIST  *b, *c;

    xcnt = sscanf(s,"%d%d%d%d%d",&x[0],&x[1],&x[2],&x[3],&x[4]);
    n = a->nevents;
    b = cscoreListCreate(csound, n);
    p = &a->e[1];
    q = &b->e[1];
    while ((n--) && (e = *p++) != NULL) {
      if (e->op != 'i')
        *q++ = e;
      else {
        insno = (int)e->p[1];
        xn = xcnt;  xp = x;
        while (xn--)
          if (*xp++ == insno) {
            *q++ = e;
            break;
          }
      }
    }
    c = cscoreListCopy(csound,b);
    csfree((CSHDR *) b);
    return(c);
}

PUBLIC EVLIST * cscoreListExtractTime(CSOUND *csound,
                 EVLIST *a, MYFLT from, MYFLT to) /* list extract by time */
{
    EVENT **p, **q, *e;
    EVLIST *b, *c;
    MYFLT maxp3;
    int  n;

    n = a->nevents;
    b = cscoreListCreate(csound,n);
    p = &a->e[1];
    q = &b->e[1];
    maxp3 = to - from;
    while ((n--) && (e = *p++) != NULL)
      switch (e->op) {
      case 'f':
        if (e->p[2] < to) {
          *q++ = e = cscoreCopyEvent(csound,e);
          b->nevents++;
          if (e->p[2] <= from)
            e->p[2] = FL(0.0);
          else e->p[2] -= from;
        }
        break;
      case 'i':
        if (e->p[2] < from) {
          if (e->p[2] + e->p[3] > from) {
            *q++ = e = cscoreCopyEvent(csound,e);
            b->nevents++;
            e->p[3] -= from - e->p[2];
            e->p[2] = FL(0.0);
            if (e->p[3] > maxp3)
              e->p[3] = maxp3;
          }
        }
        else if ((e->p[2] >= from) && (e->p[2] < to)) {
          *q++ = e = cscoreCopyEvent(csound, e);
          b->nevents++;
          if (e->p[2] + e->p[3] > to)
            e->p[3] = to - e->p[2];
          e->p[2] -= from;
        }
        break;
      default:
        *q++ = cscoreCopyEvent(csound,e);
        b->nevents++;
        break;
      }
    c = cscoreListCopy(csound,b);
    csfree((CSHDR *) b);
    return(c);
}

/* look for f statements with non-0 p[2] */

static void fp2chk(CSOUND *csound, EVLIST *a, char *s)
{
    EVENT *e, **ep = &a->e[1];
    int n = a->nevents, count = 0;

    while (n--)
      if ((e = *ep++) && e->op == 'f' && e->p[2] != 0.)
        count++;
    if (count)
      csound->Message(csound, Str("%s found %d f event%s with non-zero p2\n"),
                              s, count, count==1 ? "" : Str("s"));
}

/* separate f events from evlist */

PUBLIC EVLIST * cscoreListSeparateF(CSOUND *csound, EVLIST *a)
{
    EVLIST  *b, *c;
    EVENT   **p, **q, **r;
    int     n;

    n = a->nevents;
    b = cscoreListCreate(csound, n);
    p = q = &a->e[1];
    r = &b->e[1];
    while (n--) {
      if ((*p)->op == 'f')
        *r++ = *p++;
      else *q++ = *p++;
    }
    a->nevents = q - &a->e[1];
    b->nevents = r - &b->e[1];
    c = cscoreListCopy(csound,b);
    csfree((CSHDR *) b);
    fp2chk(csound, c, "cscoreListSeparateF");
    return(c);
}

/* separate t,w,f events from evlist */

PUBLIC EVLIST * cscoreListSeparateTWF(CSOUND *csound, EVLIST *a)
{
    EVLIST *b, *c;
    EVENT **p, **q, **r;
    int   n, op;

    n = a->nevents;
    b = cscoreListCreate(csound,n);
    p = q = &a->e[1];
    r = &b->e[1];
    while (n--) {
      if ((op = (*p)->op) == 't' || op == 'w' || op == 'f')
        *r++ = *p++;
      else *q++ = *p++;
    }
    a->nevents = q - &a->e[1];
    b->nevents = r - &b->e[1];
    c = cscoreListCopy(csound,b);
    csfree((CSHDR *) b);
    fp2chk(csound, c, "cscoreListSeparateTWF");
    return(c);
}

/* give back event space */

PUBLIC void cscoreFreeEvent(CSOUND *csound, EVENT *e)
{
    IGN(csound);
    csfree((CSHDR *) e);
}

/* give back list space */

PUBLIC void cscoreListFree(CSOUND *csound, EVLIST *a)
{
     IGN(csound);
    csfree((CSHDR *) a);
}

/* give back list and its event spaces */

PUBLIC void cscoreListFreeEvents(CSOUND *csound, EVLIST *a)
{
     IGN(csound);
    EVENT **p = &a->e[1];
    int  n = a->nevents;

    while (n--)
      csfree((CSHDR *) *p++);
    csfree((CSHDR *) a);
}

#define MAXOPEN 5

static void savinfdata(         /* store input file data */
  CSOUND *csound,
  FILE  *fp,
  EVENT *next,
  MYFLT until,
  int   wasend,
  int   warp,
  int   eof)
{
    INFILE *infp;
    int    n;

    if ((infp = infiles) == NULL) {
      infp = infiles = (INFILE *) csound->Calloc(csound, MAXOPEN * sizeof(INFILE));
      goto save;
    }
    for (n = MAXOPEN; n--; infp++)
      if (infp->iscfp == fp)
        goto save;
    for (infp = infiles, n = MAXOPEN; n--; infp++)
      if (infp->iscfp == NULL)
        goto save;
    csound->ErrorMsg(csound, Str("cscore: too many input files open"));
    exit(0);     /* FIXME: should not call exit */

 save:
    infp->iscfp = fp;
    infp->next = next;
    infp->until = until;
    infp->wasend = wasend;
    infp->warped = warp;
    infp->atEOF = eof;
}

/* make fp the cur scfp & retreive other data */
/* set nxtevtblk to subset cs.h EVTBLK struct */
/* if nxtevt buffer is empty, read one event  */

static void makecurrent(CSOUND *csound, FILE *fp)
{
    INFILE *infp;
    int    n;

    if ((infp = infiles) != NULL)
      for (n = MAXOPEN; n--; infp++)
        if (infp->iscfp == fp) {
          csound->scfp = fp;
          nxtevt = infp->next;
          nxtevtblk = (EVTBLK *) &nxtevt->strarg;
          curuntil = infp->until;
          wasend = infp->wasend;
          atEOF = infp->atEOF;
          csound->warped = infp->warped;
          if (nxtevt->op == '\0')
            if (csound->scstr == NULL ||
                csound->scstr->body[0] == '\0' ||
                !(rdscor(csound, nxtevtblk))) {
              nxtevt->op = '\0';
              atEOF = 1;
            }
          return;
        }
    csound->ErrorMsg(csound, Str("cscore: tried to set an unknown file pointer"
                                 " as the current file"));
    exit(0);     /* FIXME: should not call exit */
}

/* verify initial scfp, init other data */
/* record & make all this current       */
PUBLIC int csoundInitializeCscore(CSOUND *csound, FILE* insco, FILE* outsco)
{
    EVENT  *next;

    if (insco != NULL) {
      CORFIL *inf = corfile_create_w(csound);
      int c;
      while ((c=getc(insco))!=EOF) corfile_putc(csound, c, inf);
      corfile_rewind(inf);
      csound->scstr = inf;
    }
    if (outsco == NULL) {
      csound->ErrorMsg(csound,
                       Str("csoundInitializeCscore: no output score given."));
      return CSOUND_INITIALIZATION;
    }
    csound->scfp = insco;
    csound->oscfp = outsco;

    next = cscoreCreateEvent(csound, PMAX); /* creat EVENT blk receiving buf */
    next->op = '\0';

    savinfdata(csound, csound->scfp,
               next, FL(0.0), 1, 0, 0);/* curuntil 0, wasend, non-warp, not eof */
    makecurrent(csound, csound->scfp);  /* make all this current         */

    return CSOUND_SUCCESS;
}



/* open new cscore input file, init data */
/* & save;  no rdscor until made current */

PUBLIC FILE *cscoreFileOpen(CSOUND *csound, char *name)
{
    FILE    *fp;
    EVENT   *next;
    char    *pathname;

    /* keeping fopen() because the FILE* is returned to the user control
       program and who knows what they will do with it ... */
    pathname = csoundFindInputFile(csound, name, "INCDIR");
    if (pathname == NULL || (fp = fopen(pathname, "r")) == NULL) {
      csound->ErrorMsg(csound, Str("cscoreFileOpen: error opening %s"), name);
      exit(0);     /* FIXME: should not call exit */
    }
    csoundNotifyFileOpened(csound, pathname, CSFTYPE_SCORE, 0, 0);
    csound->Free(csound, pathname);
    /* alloc a receiving evtblk */
    next = cscoreCreateEvent(csound,PMAX);  /* FIXME: need next->op = '\0' ?? */
    /* save all, wasend, non-warped, not eof */
    savinfdata(csound, fp, next, FL(0.0), 1, 0, 0);
    return(fp);
}

PUBLIC void cscoreFileClose(CSOUND *csound, FILE *fp)
{
    INFILE *infp;
    int n;

    if (fp == NULL) {
      csound->Message(csound, Str("cscoreFileClose: NULL file pointer\n"));
      return;
    }
    if ((infp = infiles) != NULL)
      for (n = MAXOPEN; n--; infp++)
        if (infp->iscfp == fp) {
          infp->iscfp = NULL;
          csound->Free(csound, (char *)infp->next);
          fclose(fp);
          if (csound->scfp == fp) csound->scfp = NULL;
          return;
        }
    csound->Message(csound, Str("cscoreFileClose: fp not recorded\n"));
}

PUBLIC FILE *cscoreFileGetCurrent(CSOUND *csound)
{
    if (csound->scfp == NULL) {
      csound->ErrorMsg(csound, Str("cscoreFileGetCurrent: no fp current"));
      exit(0);     /* FIXME: should not call exit */
    }
    return(csound->scfp);
}

/* save the current infil states */
/* make fp & its states current  */

PUBLIC void cscoreFileSetCurrent(CSOUND *csound, FILE *fp)
{
    if (fp != NULL) {
      CORFIL *inf = corfile_create_w(csound);
      int c;
      fseek(fp, 0, SEEK_SET);
      while ((c=getc(fp))!=EOF) corfile_putc(csound, c, inf);
      corfile_rewind(inf);
      corfile_rm(csound, &csound->scstr);
      csound->scstr = inf;
      nxtevt->op = '\0';
      atEOF = 0;
    }
    if (csound->scfp != NULL)
      savinfdata(csound,
                 csound->scfp, nxtevt, curuntil, wasend, csound->warped, atEOF);
    makecurrent(csound, fp);
}

/* count entries in event list */

PUBLIC int cscoreListCount(CSOUND *csound, EVLIST *a)
{
     IGN(csound);
    EVENT **p;
    int  n, nrem;

    n = 0;
    nrem = a->nslots;
    p = &a->e[1];
    while ((nrem--) && *p++ != NULL)
      n++;
    return(n);
}

