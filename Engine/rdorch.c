/*
  rdorch.c:

  Copyright (C) 1991-2002 Barry Vercoe, John ffitch, Istvan Varga

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

#include "csoundCore.h"         /*                      RDORCH.C        */
#include <ctype.h>
#include "namedins.h"   /* IV - Oct 31 2002 */
#include "typetabl.h"   /* IV - Oct 31 2002 */
#include "envvar.h"
#include <stddef.h>
#include "corfile.h"

#ifdef sun
#define   SEEK_SET        0
#define   SEEK_CUR        1
#define   SEEK_END        2
#endif

#define LINMAX    1000
#define LENMAX    4096L
#define GRPMAX    VARGMAX
#define LBLMAX    100

//#define MACDEBUG (1)

typedef struct  {
  int     reqline;
  char    *label;
} LBLREQ;

#define MARGS   (3)

typedef struct MACRO {          /* To store active macros */
  char          *name;        /* Use is by name */
  int           acnt;         /* Count of arguments */
  char          *body;        /* The text of the macro */
  struct MACRO  *next;        /* Chain of active macros */
  int           margs;        /* amount of space for args */
  char          *arg[MARGS];  /* With these arguments */
} MACRO;

typedef struct in_stack {
  int16   string;
  int16   args;
  char    *body;
  FILE    *file;
  void    *fd;
  MACRO   *mac;
  int     line;
  int     unget_cnt;
  char    unget_buf[128];
} IN_STACK;

typedef struct iflabel {            /* for if/else/endif */
  char    els[256];
  char    end[256];
  /* is the conditional valid at i-time ? 0: no, 1: yes, -1: unknown */
  int     ithen;
  struct  iflabel *prv;
} IFLABEL;

typedef struct IFDEFSTACK_ {
  struct IFDEFSTACK_  *prv;
  unsigned char   isDef;      /* non-zero if #ifdef is true, or #ifndef   */
  /*   is false                               */
  unsigned char   isElse;     /* non-zero between #else and #endif        */
  unsigned char   isSkip;     /* sum of: 1: skipping code due to this     */
  /*   #ifdef, 2: skipping due to parent      */
} IFDEFSTACK;

typedef struct {
  MACRO   *macros;
  int32    lenmax /* = LENMAX */;  /* Length of input line buffer  */
  char    *ortext;
  char    **linadr;               /* adr of each line in text     */
  int     curline;                /* current line being examined  */
  char    *collectbuf;            /* splitline collect buffer     */
  char    **group;                /* splitline local storage      */
  char    **grpsav;               /* copy of above                */
  int32    grpmax /* = GRPMAX */;  /* Size of group structure      */
  int     opgrpno;                /* grpno identified as opcode   */
  int     linopnum;               /* data for opcode in this line */
  char    *linopcod;
  int     linlabels;              /* count of labels this line    */
  LBLREQ  *lblreq;
  int     lblmax;
  int     lblcnt;
  int     lgprevdef;
  int     opnum;                  /* opcod data carriers          */
  char    *opcod;                 /*  (line or subline)           */
  ARGLST  *nxtarglist, *nullist;
  IN_STACK  *inputs, *str;
  FILE    *fp;
  void    *fd;
  int     input_size, input_cnt;
  int     pop;                    /* Number of macros to pop      */
  int     ingappop /* = 1 */;
  int     linepos /* = -1 */;
  int32    *typemask_tabl;
  int32    *typemask_tabl_in, *typemask_tabl_out;
  int32    orchsiz;
  IFLABEL *iflabels;
  int     repeatingElseifLine;
  int32    tempNum /* = 300L */;
  int     repeatingElseLine;
  int16   grpcnt, nxtest /* = 1 */;
  int16   xprtstno, polcnt;
  int16   instrblk, instrcnt;
  int16   opcodblk;               /* IV - Sep 8 2002 */
  int16   opcodflg;               /* 1: xin, 2: xout, 4: setksmps */
  IFDEFSTACK  *ifdefStack;
  TEXT    optext;                 /* struct to be passed back to caller */
} RDORCH_GLOBALS;

#define ST(x)   (((RDORCH_GLOBALS*) csound->rdorchGlobals)->x)
#define CURLINE (csound->oparms->useCsdLineCounts ? \
                 csound->orcLineOffset + ST(curline) : ST(curline))

static inline int isNameChar(int c, int pos)
{
  c = (int) ((unsigned char) c);
  return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}


/* The fromScore parameter should be 1 if opening a score include file,
   0 if opening an orchestra include file */
void *fopen_path(CSOUND *csound, FILE **fp, char *name, char *basename,
                 char *env, int fromScore)
{
  void *fd;
  int  csftype = (fromScore ? CSFTYPE_SCO_INCLUDE : CSFTYPE_ORC_INCLUDE);

  /* First try to open name given */
  fd = csound->FileOpen2(csound, fp, CSFILE_STD, name, "rb", NULL,
                         csftype, 0);
  if (fd != NULL)
    return fd;
  /* if that fails try in base directory */
  if (basename != NULL) {
    char *dir, *name_full;
    if ((dir = csoundSplitDirectoryFromPath(csound, basename)) != NULL) {
      name_full = csoundConcatenatePaths(csound, dir, name);
      fd = csound->FileOpen2(csound, fp, CSFILE_STD, name_full, "rb", NULL,
                             csftype, 0);
      mfree(csound, dir);
      mfree(csound, name_full);
      if (fd != NULL)
        return fd;
    }
  }
  /* or use env argument */
  fd = csound->FileOpen2(csound, fp, CSFILE_STD, name, "rb", env,
                         csftype, 0);
  return fd;
}


static int pnum(char *s)        /* check a char string for pnum format  */
/*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
  int n;

  if (*s == 'p' || *s == 'P')
    if (sscanf(++s, "%d", &n))
      return(n);
  return(-1);
}

char argtyp(CSOUND *csound, char *s)
{                       /* find arg type:  d, w, a, k, i, c, p, r, S, B, b */
  char c = *s;        /*   also set lgprevdef if !c && !p && !S */


  /*trap this before parsing for a number! */
  /* two situations: defined at header level: 0dbfs = 1.0
   *  and returned as a value:  idb = 0dbfs
   */
  if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
      (c == '0' && strcmp(s, "0dbfs") != 0))
    return('c');                              /* const */
  if (pnum(s) >= 0)
    return('p');                              /* pnum */
  if (c == '"')
    return('S');                              /* quoted String */
  ST(lgprevdef) = lgexist(csound, s);               /* (lgprev) */
  if (strcmp(s,"sr") == 0    || strcmp(s,"kr") == 0 ||
      strcmp(s,"0dbfs") == 0 || strcmp(s,"nchnls_i") == 0 ||
      strcmp(s,"ksmps") == 0 || strcmp(s,"nchnls") == 0)
    return('r');                              /* rsvd */
  if (c == 'w')               /* N.B. w NOT YET #TYPE OR GLOBAL */
    return(c);
  if (c == '#')
    c = *(++s);
  if (c == 'g')
    c = *(++s);
  if (strchr("akiBbfS", c) != NULL)
    return(c);
  else return('?');
}

void synterr(CSOUND *csound, const char *s, ...)
{
  va_list args;

  csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error:  "));
  va_start(args, s);
  csound->MessageV(csound, CSOUNDMSG_ERROR, s, args);
  va_end(args);


  /* FIXME - Removed temporarily for debugging
   * This function may not be necessary at all in the end if some of this is
   * done in the parser
   */
  csound->synterrcnt++;
}



