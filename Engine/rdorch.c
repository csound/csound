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

static  void    intyperr(CSOUND *, int, char, char);
static  void    printgroups(CSOUND *, int);
static  int     isopcod(CSOUND *, char *);
static  void    lblrequest(CSOUND *, char *), lblfound(CSOUND *, char *);
static  void    lblclear(CSOUND *), lblchk(CSOUND *);
static  void    lexerr(CSOUND *, const char *, ...);
static  void    synterrp(CSOUND *, const char *, char *);

static ARGLST *copy_arglist(CSOUND *csound, ARGLST *old)
{
  size_t n = sizeof(ARGLST) + old->count * sizeof(char*) - sizeof(char*);
  ARGLST *nn = (ARGLST*) mmalloc(csound, n);
  memcpy(nn, old, n);
  memset(old, 0, n);
  return nn;
}

static inline int isNameChar(int c, int pos)
{
  c = (int) ((unsigned char) c);
  return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

/* Functions to read/unread chracters from
 * a stack of file and macro inputs */

static inline void ungetorchar(CSOUND *csound, int c)
{
  if (LIKELY(ST(str)->unget_cnt < 128))
    ST(str)->unget_buf[ST(str)->unget_cnt++] = (char) c;
  else
    csoundDie(csound, Str("ungetorchar(): buffer overflow"));
}

static int skiporccomment(CSOUND *csound)
{
  int c;
  int mode = 0;               /* Mode = 1 after / character */
  int srccnt = 0;
 top:
  if (ST(str)->unget_cnt) {
    c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
  }
  else if (ST(str)->string) {
    c = *ST(str)->body++;
    if (c == '\0') {
      ST(pop) += ST(str)->args;
      ST(str)--; ST(input_cnt)--;
      ST(linepos) = -1;
      return srccnt;
    }
  }
  else {
    c = getc(ST(str)->file);
    if (c == EOF) {
      if (ST(str) == &ST(inputs)[0]) {
        ST(linepos) = -1;
        return srccnt;
      }
      if (ST(str)->fd != NULL) {
        csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
      }
      ST(str)--; ST(input_cnt)--;
      ST(str)->line++; ST(linepos) = -1;
      return srccnt;
    }
  }
  if (c == '*') mode = 1;     /* look for end of comment */
  else if (c == '/' && mode == 1) {
    return srccnt;
  }
  else mode = 0;
  if (c == '\n') {
    ST(str)->line++; ST(linepos) = -1;
    srccnt++;
  }
  goto top;
}

static void skiporchar(CSOUND *csound)
{
  int c;
 top:
  if (UNLIKELY(ST(str)->unget_cnt)) {
    c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
    if (c == '\n') {
      ST(linepos) = -1;
      return;
    }
    goto top;
  }
  else if (ST(str)->string) {
    c = *ST(str)->body++;
    if (c == '\n') {
      ST(str)->line++; ST(linepos) = -1;
      return;
    }
    if (c == '\0') {
      ST(pop) += ST(str)->args;
      ST(str)--; ST(input_cnt)--;
      ST(linepos) = -1;
      return;
    }
  }
  else {
    c = getc(ST(str)->file);
    if (c == '\n' || c == '\r' || c == 26) {    /* MS-DOS spare ^Z */
      ST(str)->line++; ST(linepos) = -1;
      if (c == '\r') {
        if (ST(str)->string) {
          if ((c = *ST(str)->body++) != '\n')
            ST(str)->body--;
        }
        else if ((c = getc(ST(str)->file)) != '\n')
          ungetc(c, ST(str)->file);
      }
      return;
    }
    if (UNLIKELY(c == EOF)) {
      if (ST(str) == &ST(inputs)[0]) {
        ST(linepos) = -1;
        return;
      }
      if (ST(str)->fd != NULL) {
        csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
      }
      ST(str)--; ST(input_cnt)--;
      ST(str)->line++; ST(linepos) = -1;
      return;
    }
  }
  ST(linepos)++;
  goto top;
}

static int getorchar(CSOUND *csound)
{
  int c;
 top:
  if (UNLIKELY(ST(str)->unget_cnt)) {
    c = (int) ((unsigned char) ST(str)->unget_buf[--ST(str)->unget_cnt]);
    if (c == '\n')
      ST(linepos) = -1;
    //    printf("%s(%d): %c(%.2x)\n", __FILE__, __LINE__, c,c);
    return c;
  }
  else if (ST(str)->string) {
    c = *ST(str)->body++;
    if (UNLIKELY(c == '\0')) {
      if (ST(str) == &ST(inputs)[0]) {
        //corfile_rm(&(csound->orchstr));
        //        printf("%s(%d): EOF\n", __FILE__, __LINE__);
        return EOF;
      }
      ST(pop) += ST(str)->args;
      ST(str)--; ST(input_cnt)--;
      goto top;
    }
  }
  else {
    c = getc(ST(str)->file);
    if (UNLIKELY(c == 26)) goto top;    /* MS-DOS spare ^Z */
    if (UNLIKELY(c == EOF)) {
      if (ST(str) == &ST(inputs)[0]) return EOF;
      if (ST(str)->fd != NULL) {
        csound->FileClose(csound, ST(str)->fd); ST(str)->fd = NULL;
      }
      ST(str)--; ST(input_cnt)--; goto top;
    }
  }
  if (c == '\r') {
    int d;
    if (ST(str)->string) {
      if ((d = *ST(str)->body++) != '\n')
        ST(str)->body--;
    }
    else if ((d = getc(ST(str)->file)) != '\n') {
      ungetc(d, ST(str)->file);
    }
    c = '\n';
  }
  if (c == '\n') {
    ST(str)->line++; ST(linepos) = -1;
  }
  else ST(linepos)++;
  if (ST(ingappop) && ST(pop)) {
    do {
      MACRO *nn = ST(macros)->next;
      int i;
#ifdef MACDEBUG
      csound->Message(csound, "popping %s\n", ST(macros)->name);
#endif
      mfree(csound, ST(macros)->name); mfree(csound, ST(macros)->body);
      for (i=0; i<ST(macros)->acnt; i++)
        mfree(csound, ST(macros)->arg[i]);
      mfree(csound, ST(macros));
      ST(macros) = nn;
      ST(pop)--;
    } while (ST(pop));
  }
  //  printf("%s(%d): %c(%.2x)\n", __FILE__, __LINE__, c,c);
  return c;
}

static int getorchar_noeof(CSOUND *csound)
{
  int     c;

  c = getorchar(csound);
  if (UNLIKELY(c == EOF))
    lexerr(csound, Str("Unexpected end of orchestra file"));
  return c;
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

static void add_math_const_macro(CSOUND *csound, char * name, char *body)
{
  MACRO *mm;

  mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
  mm->name = (char*) mcalloc(csound, strlen(name) + 3);
  sprintf(mm->name, "M_%s", name);
  mm->next = ST(macros);
  ST(macros) = mm;
  mm->margs = MARGS;    /* Initial size */
  mm->acnt = 0;
  mm->body = (char*) mcalloc(csound, strlen(body) + 1);
  mm->body = strcpy(mm->body, body);
}

/**
 * Add math constants from math.h as orc macros
 */
static void init_math_constants_macros(CSOUND *csound)
{
  add_math_const_macro(csound, "E", "2.7182818284590452354");
  add_math_const_macro(csound, "LOG2E", "1.4426950408889634074");
  add_math_const_macro(csound, "LOG10E", "0.43429448190325182765");
  add_math_const_macro(csound, "LN2", "0.69314718055994530942");
  add_math_const_macro(csound, "LN10", "2.30258509299404568402");
  add_math_const_macro(csound, "PI", "3.14159265358979323846");
  add_math_const_macro(csound, "PI_2", "1.57079632679489661923");
  add_math_const_macro(csound, "PI_4", "0.78539816339744830962");
  add_math_const_macro(csound, "1_PI", "0.31830988618379067154");
  add_math_const_macro(csound, "2_PI", "0.63661977236758134308");
  add_math_const_macro(csound, "2_SQRTPI", "1.12837916709551257390");
  add_math_const_macro(csound, "SQRT2", "1.41421356237309504880");
  add_math_const_macro(csound, "SQRT1_2", "0.70710678118654752440");
  add_math_const_macro(csound, "INF", "800000000000.0"); /* ~25367 years */
}

static void init_omacros(CSOUND *csound, NAMES *nn)
{
  while (nn) {
    char  *s = nn->mac;
    char  *p = strchr(s, '=');
    char  *mname;
    MACRO *mm;

    if (p == NULL)
      p = s + strlen(s);
    if (csound->oparms->msglevel & 7)
      csound->Message(csound, Str("Macro definition for %*s\n"), p - s, s);
    s = strchr(s, ':') + 1;                   /* skip arg bit */
    if (UNLIKELY(s == NULL || s >= p))
      csound->Die(csound, Str("Invalid macro name for --omacro"));
    mname = (char*) mmalloc(csound, (p - s) + 1);
    strncpy(mname, s, p - s);
    mname[p - s] = '\0';
    /* check if macro is already defined */
    for (mm = ST(macros); mm != NULL; mm = mm->next) {
      if (strcmp(mm->name, mname) == 0)
        break;
    }
    if (mm == NULL) {
      mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
      mm->name = mname;
      mm->next = ST(macros);
      ST(macros) = mm;
    }
    else
      mfree(csound, mname);
    mm->margs = MARGS;    /* Initial size */
    mm->acnt = 0;
    if (*p != '\0')
      p++;
    mm->body = (char*) mmalloc(csound, strlen(p) + 1);
    strcpy(mm->body, p);
    nn = nn->next;
  }
}

void rdorchfile(CSOUND *csound)     /* read entire orch file into txt space */
{
  int     c, lincnt;
  int     srccnt;
  char    *cp, *endspace, *ortext;
  int     linmax = LINMAX;        /* Maximum number of lines      */
  int     heredoc = 0, openquote = 0;

  if (csound->rdorchGlobals == NULL) {
    csound->rdorchGlobals = csound->Calloc(csound, sizeof(RDORCH_GLOBALS));
    ST(lenmax)    = LENMAX;
    ST(grpmax)    = GRPMAX;
    ST(ingappop)  = 1;
    ST(linepos)   = -1;
    ST(tempNum)   = 300L;
    ST(nxtest)    = 1;
  }
  init_math_constants_macros(csound);
  init_omacros(csound, csound->omacros);
  /* IV - Oct 31 2002: create tables for easier checking for common types */
  if (!ST(typemask_tabl)) {
    const int32 *ptr = typetabl1;
    ST(typemask_tabl) = (int32*) mcalloc(csound, sizeof(int32) * 256);
    ST(typemask_tabl_in) = (int32*) mcalloc(csound, sizeof(int32) * 256);
    ST(typemask_tabl_out) = (int32*) mcalloc(csound, sizeof(int32) * 256);
    while (*ptr) {            /* basic types (both for input */
      int32 pos = *ptr++;      /* and output) */
      ST(typemask_tabl)[pos] = ST(typemask_tabl_in)[pos] =
        ST(typemask_tabl_out)[pos] = *ptr++;
    }
    ptr = typetabl2;
    while (*ptr) {            /* input types */
      int32 pos = *ptr++;
      ST(typemask_tabl_in)[pos] = *ptr++;
    }
    ptr = typetabl3;
    while (*ptr) {            /* output types */
      int32 pos = *ptr++;
      ST(typemask_tabl_out)[pos] = *ptr++;
    }
  }
  csound->Message(csound, Str("orch compiler:\n"));
  ST(inputs) = (IN_STACK*) mmalloc(csound, 20 * sizeof(IN_STACK));
  ST(input_size) = 20;
  ST(input_cnt) = 0;
  ST(str) = ST(inputs);
  ST(str)->line = 1;
  ST(str)->unget_cnt = 0;
  if (csound->orchstr) {
    ST(orchsiz) = corfile_length(csound->orchstr);
    ST(str)->string = 1;
    ST(str)->body = corfile_body(csound->orchstr);
    ST(str)->file = NULL;
    ST(str)->fd = NULL;
  }
  else {
    /* if (UNLIKELY((ST(fd) = csound->FileOpen2(csound, &ST(fp), CSFILE_STD, */
    /*                         csound->orchname, "rb", NULL, CSFTYPE_ORCHESTRA, */
    /*                                        (csound->tempStatus & csOrcMask)!=0)) == NULL)) */
    csoundDie(csound, Str("cannot open orch file %s"), csound->orchname);
    /* if (UNLIKELY(fseek(ST(fp), 0L, SEEK_END) != 0)) */
    /*   csoundDie(csound, Str("cannot find end of file %s"), csound->orchname); */
    /* if (UNLIKELY((ST(orchsiz) = ftell(ST(fp))) <= 0)) */
    /*   csoundDie(csound, Str("ftell error on %s"), csound->orchname); */
    /* rewind(ST(fp)); */
    /* ST(str)->string = 0; */
    /* ST(str)->file = ST(fp); */
    /* ST(str)->fd = ST(fd); */
    /* ST(str)->body = csound->orchname; */
  }
  ortext = mmalloc(csound, ST(orchsiz) + 1);          /* alloc mem spaces */
  ST(linadr) = (char **) mmalloc(csound, (LINMAX + 1) * sizeof(char *));
  strsav_create(csound);
  lincnt = srccnt = 1;
  cp = ST(linadr)[1] = ortext;
  endspace = ortext + ST(orchsiz) + 1;
  strsav_string(csound, "sr");
  ST(group) = (char **)mcalloc(csound, (GRPMAX+1)*sizeof(char*));
  ST(grpsav)= (char **)mcalloc(csound, (GRPMAX+1)*sizeof(char*));
  ST(lblreq) = (LBLREQ*)mcalloc(csound, LBLMAX*sizeof(LBLREQ));
  ST(lblmax) = LBLMAX;

 top:
  while ((c = getorchar(csound)) != EOF) {    /* read entire orch file  */
    if (cp == endspace-5) {                   /* Must extend */
      char *orold = ortext;
      int  i;
      /* printf("Expand orch: %p (%d) %p -> ", ortext, ST(orchsiz), endspace); */
      ST(orchsiz) = ST(orchsiz) + (ST(orchsiz) >> 4) + 1L;
      ST(orchsiz) = (ST(orchsiz) + 511L) & (~511L);
      ortext = mrealloc(csound, ortext, ST(orchsiz));
      endspace = ortext + ST(orchsiz) + 1;
      /* printf("%p (%d) %p\n", ortext, ST(orchsiz), endspace); */
      if (ortext != orold) {
        ptrdiff_t adj = ortext - orold;
        for (i=1; i<=lincnt; i++)
          ST(linadr)[i] += adj; /* Relocate */
        cp += adj;
      }
    }
    *cp++ = c;
    if (c == '{' && !openquote) {
      char  c2 = getorchar(csound);
      if (c2 == '{') {
        heredoc = 1;
        *cp++ = c;
      }
      else
        ungetorchar(csound, c2);
    }
    else if (c == '}' && heredoc) {
      char  c2 = getorchar(csound);
      if (c2 == '}') {
        heredoc = 0;
        *cp++ = c;
      }
      else
        ungetorchar(csound, c2);
    }
    if (c == ';' && !heredoc) {
      skiporchar(csound);
      *(cp - 1) = (char) (c = '\n');
    }
    if (c == '"' && !heredoc) {
      openquote = !openquote;
    }
    if (c == '\\' && !heredoc & !openquote) {      /* Continuation ?       */
      while ((c = getorchar(csound)) == ' ' || c == '\t')
        ;                                          /* Ignore spaces        */
      if (c == ';') {                              /* Comments get skipped */
        skiporchar(csound);
        c = '\n';
      }
      if (c == '\n') {
        cp--;                                      /* Ignore newline */
        srccnt++;                                  /*    record a fakeline */
        /* lincnt++; Thsi is wrong */
      }
      else {
        *cp++ = c;
      }
    }
    else if (c == '/') {
      c = getorchar(csound);
      if (c=='*') {
        srccnt += skiporccomment(csound);
        cp--;                 /* ?? ?? ?? */
        goto top;
      }
      else {
        ungetorchar(csound, c);
        c = '/';
      }
    }
    else if (c == '\n') {                          /* at each new line */
      char *lp = ST(linadr)[lincnt];
      /* printf("lincnt=%d; lp=%p, ST(linadr)=%p\n", lincnt, lp, ST(linadr)); */
      while ((c = *lp) == ' ' || c == '\t')
        lp++;
      if (*lp != '\n' && *lp != ';') {
        ST(curline) = lincnt - 1;
      }
      srccnt++;
      if (++lincnt >= linmax) {
        linmax += 100;
        ST(linadr) = (char**) mrealloc(csound, ST(linadr), (linmax + 1)
                                       * sizeof(char*));
      }
      /*    ST(srclin)[lincnt] = srccnt;    unused  */
      ST(linadr)[lincnt] = cp;            /* record the adrs */
    }
    else if (c == '#' && ST(linepos) == 0 && !heredoc) {
      /* Start Macro definition */
      /* also deal with #include here */
      char  *mname, *preprocName;
      int mlen = 40;
      int   i, cnt;
      mname = (char  *)malloc(mlen);
      cp--;
    parsePreproc:
      preprocName = NULL;
      i = 0;
      cnt = 0;
      mname[cnt++] = '#';
      if (cnt==mlen)
        mname = (char *)realloc(mname, mlen+=40);
      do {
        c = getorchar(csound);
        if (UNLIKELY(c == EOF))
          break;
        mname[cnt++] = c;
        if (cnt==mlen)
          mname = (char *)realloc(mname, mlen+=40);
      } while ((c == ' ' || c == '\t'));
      mname[cnt] = '\0';
      if (c == EOF || c == '\n')
        goto unknownPreproc;
      preprocName = &(mname[cnt - 1]);
      while (1) {
        c = getorchar(csound);
        if (c == EOF || !(isalnum(c) || c == '_'))
          break;
        mname[cnt++] = c;
        if (cnt==mlen)
          mname = (char *)realloc(mname, mlen+=40);
      }
      mname[cnt] = '\0';
      if (strcmp(preprocName, "define") == 0 &&
          !(ST(ifdefStack) != NULL && ST(ifdefStack)->isSkip)) {
        MACRO *mm = (MACRO*) mmalloc(csound, sizeof(MACRO));
        int   arg = 0;
        int   size = 40;
        mm->margs = MARGS;    /* Initial size */
        while (isspace((c = getorchar(csound))))
          ;
        while (isNameChar(c, i)) {
          mname[i++] = c;
          if (i==mlen)
            mname = (char *)realloc(mname, mlen+=40);
          c = getorchar(csound);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel & 7)
          csound->Message(csound, Str("Macro definition for %s\n"), mname);
        mm->name = mmalloc(csound, i + 1);
        strcpy(mm->name, mname);
        if (c == '(') {       /* arguments */
#ifdef MACDEBUG
          csound->Message(csound, "M-arguments: ");
#endif
          do {
            while (isspace((c = getorchar_noeof(csound))))
              ;
            i = 0;
            while (isNameChar(c, i)) {
              mname[i++] = c;
              if (i==mlen)
                mname = (char *)realloc(mname, mlen+=40);
              c = getorchar(csound);
            }
            mname[i] = '\0';
#ifdef MACDEBUG
            csound->Message(csound, "%s\t", mname);
#endif
            mm->arg[arg] = mmalloc(csound, i + 1);
            strcpy(mm->arg[arg++], mname);
            if (arg >= mm->margs) {
              mm = (MACRO*) mrealloc(csound, mm, sizeof(MACRO)
                                     + mm->margs * sizeof(char*));
              mm->margs += MARGS;
            }
            while (isspace(c))
              c = getorchar_noeof(csound);
          } while (c == '\'' || c == '#');
          if (UNLIKELY(c != ')'))
            csound->Message(csound, Str("macro error\n"));
        }
        mm->acnt = arg;
        i = 0;
        while (c != '#')
          c = getorchar_noeof(csound);        /* Skip to next # */
        mm->body = (char*) mmalloc(csound, 100);
        while ((c = getorchar_noeof(csound)) != '#') {
          mm->body[i++] = c;
          if (UNLIKELY(i >= size))
            mm->body = mrealloc(csound, mm->body, size += 100);
          if (c == '\\') {                    /* allow escaped # */
            mm->body[i++] = c = getorchar_noeof(csound);
            if (UNLIKELY(i >= size))
              mm->body = mrealloc(csound, mm->body, size += 100);
          }
          if (c == '\n')
            srccnt++;
        }
        mm->body[i] = '\0';
        mm->next = ST(macros);
        ST(macros) = mm;
#ifdef MACDEBUG
        csound->Message(csound, "Macro %s with %d arguments defined\n",
                        mm->name, mm->acnt);
#endif
        c = ' ';
      }
      else if (strcmp(preprocName, "include") == 0 &&
               !(ST(ifdefStack) != NULL && ST(ifdefStack)->isSkip)) {
        int   delim;
        while (isspace(c))
          c = getorchar(csound);
        delim = c;
        i = 0;
        while ((c = getorchar_noeof(csound)) != delim) {
          mname[i++] = c;
          if (i==mlen)
            mname = (char *)realloc(mname, mlen+=40);
        }
        mname[i] = '\0';
        do {
          c = getorchar(csound);
        } while (c != EOF && c != '\n');
#ifdef MACDEBUG
        csound->Message(csound, "#include \"%s\"\n", mname);
#endif
        ST(input_cnt)++;
        if (ST(input_cnt) >= ST(input_size)) {
          ST(input_size) += 20;
          ST(inputs) = mrealloc(csound, ST(inputs), ST(input_size)
                                * sizeof(IN_STACK));
        }
        ST(str) = (IN_STACK*) ST(inputs) + (int) ST(input_cnt);
        ST(str)->string = 0;
        ST(str)->fd = fopen_path(csound, &(ST(str)->file),
                                 mname, csound->orchname, "INCDIR", 0);
        if (UNLIKELY(ST(str)->fd == NULL)) {
          csound->Message(csound,
                          Str("Cannot open #include'd file %s\n"), mname);
          /* Should this stop things?? */
          ST(str)--; ST(input_cnt)--;
        }
        else {
          ST(str)->body = csound->GetFileName(ST(str)->fd);
          ST(str)->line = 1;
          ST(str)->unget_cnt = 0;
          ST(linepos) = -1;
        }
      }
      else if (strcmp(preprocName, "ifdef") == 0 ||
               strcmp(preprocName, "ifndef") == 0) {
        MACRO   *mm;                  /* #ifdef or #ifndef */
        IFDEFSTACK  *pp;
        pp = (IFDEFSTACK*) mcalloc(csound, sizeof(IFDEFSTACK));
        pp->prv = ST(ifdefStack);
        if (strcmp(preprocName, "ifndef") == 0)
          pp->isDef = 1;
        while (isspace(c = getorchar(csound)))
          ;
        while (isNameChar(c, i)) {
          mname[i++] = c;
          if (i==mlen)
            mname = (char *)realloc(mname, mlen+=40);
          c = getorchar(csound);
        }
        mname[i] = '\0';
        for (mm = ST(macros); mm != NULL; mm = mm->next) {
          if (strcmp(mname, mm->name) == 0) {
            pp->isDef ^= (unsigned char) 1;
            break;
          }
        }
        ST(ifdefStack) = pp;
        pp->isSkip = pp->isDef ^ (unsigned char) 1;
        if (pp->prv != NULL && pp->prv->isSkip)
          pp->isSkip |= (unsigned char) 2;
        if (!pp->isSkip) {
          while (c != '\n' && c != EOF) {     /* Skip to end of line */
            c = getorchar(csound);
          }
          srccnt++; goto top;
        }
        else {                                /* Skip a section of code */
        ifdefSkipCode:
          do {
            while (c != '\n') {
              if (UNLIKELY(c == EOF))
                lexerr(csound, Str("unmatched #ifdef"));
              c = getorchar(csound);
            }
            srccnt++;
            c = getorchar(csound);
          } while (c != '#');
          goto parsePreproc;
        }
      }
      else if (strcmp(preprocName, "else") == 0) {
        if (ST(ifdefStack) == NULL || ST(ifdefStack)->isElse)
          lexerr(csound, Str("Unmatched #else"));
        while (c != '\n' && c != EOF)
          c = getorchar(csound);
        srccnt++;
        ST(ifdefStack)->isElse = 1;
        ST(ifdefStack)->isSkip ^= (unsigned char) 1;
        if (ST(ifdefStack)->isSkip)
          goto ifdefSkipCode;
        goto top;
      }
      else if (strcmp(preprocName, "end") == 0 ||
               strcmp(preprocName, "endif") == 0) {
        IFDEFSTACK  *pp = ST(ifdefStack);
        if (UNLIKELY(pp == NULL))
          lexerr(csound, Str("Unmatched #endif"));
        while (c != '\n' && c != EOF) {
          c = getorchar(csound);
        }
        srccnt++;
        ST(ifdefStack) = pp->prv;
        mfree(csound, pp);
        if (ST(ifdefStack) != NULL && ST(ifdefStack)->isSkip)
          goto ifdefSkipCode;
        goto top;
      }
      else if (strcmp(preprocName, "undef") == 0 &&
               !(ST(ifdefStack) != NULL && ST(ifdefStack)->isSkip)) {
        while (isspace(c = getorchar(csound)))
          ;
        while (isNameChar(c, i)) {
          mname[i++] = c;
          if (i==mlen)
            mname = (char *)realloc(mname, mlen+=40);
          c = getorchar(csound);
        }
        mname[i] = '\0';
        if (csound->oparms->msglevel)
          csound->Message(csound,Str("macro %s undefined\n"), mname);
        if (strcmp(mname, ST(macros)->name)==0) {
          MACRO *mm=ST(macros)->next;
          mfree(csound, ST(macros)->name); mfree(csound, ST(macros)->body);
          for (i=0; i<ST(macros)->acnt; i++)
            mfree(csound, ST(macros)->arg[i]);
          mfree(csound, ST(macros)); ST(macros) = mm;
        }
        else {
          MACRO *mm = ST(macros);
          MACRO *nn = mm->next;
          while (strcmp(mname, nn->name) != 0) {
            mm = nn; nn = nn->next;
            if (nn == NULL)
              lexerr(csound, Str("Undefining undefined macro"));
          }
          mfree(csound, nn->name); mfree(csound, nn->body);
          for (i=0; i<nn->acnt; i++)
            mfree(csound, nn->arg[i]);
          mm->next = nn->next; mfree(csound, nn);
        }
        while (c != '\n' && c != EOF)
          c = getorchar(csound);              /* ignore rest of line */
        srccnt++;
      }
      else {
      unknownPreproc:
        if (ST(ifdefStack) != NULL && ST(ifdefStack)->isSkip)
          goto ifdefSkipCode;
        if (preprocName == NULL)
          lexerr(csound, Str("Unexpected # character"));
        else if (strcmp("exit", preprocName)) /* VL: ignore #exit */
          lexerr(csound, Str("Unknown # option: '%s'"), preprocName);
      }
      free(mname);
    }
    else if (c == '$' && !heredoc) {
      char      name[100];
      int       i = 0;
      int       j;
      MACRO     *mm, *mm_save = NULL;
      ST(ingappop) = 0;
      while (isNameChar((c = getorchar(csound)), i)) {
        name[i++] = c; name[i] = '\0';
        mm = ST(macros);
        while (mm != NULL) {  /* Find the definition */
          if (!(strcmp(name, mm->name))) {
            mm_save = mm;     /* found a match, save it */
            break;
          }
          mm = mm->next;
        }
      }
      mm = mm_save;
      if (UNLIKELY(mm == NULL)) {
        if (i)
          lexerr(csound,Str("Undefined macro: '%s'"), name);
        else
          lexerr(csound,Str("Macro expansion symbol ($) without macro name"));
        continue;
      }
      if ((int) strlen(mm->name) != i) {
        int cnt = i - (int) strlen(mm->name);
        csound->Warning(csound, Str("$%s matches macro name $%s"),
                        name, mm->name);
        do {
          ungetorchar(csound, c);
          c = name[--i];
        } while (cnt--);
      }
      else if (c != '.')
        ungetorchar(csound, c);
#ifdef MACDEBUG
      csound->Message(csound, "Found macro %s required %d arguments\n",
                      mm->name, mm->acnt);
#endif
      /* Should bind arguments here */
      /* How do I recognise entities?? */
      if (mm->acnt) {
        if (UNLIKELY((c = getorchar(csound)) != '('))
          lexerr(csound, Str("Syntax error in macro call"));
        for (j = 0; j < mm->acnt; j++) {
          char  term = (j == mm->acnt - 1 ? ')' : '\'');
          char  trm1 = (j == mm->acnt - 1 ? ')' : '#');   /* Compatability */
          MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
          int   size = 100;
          nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
          strcpy(nn->name, mm->arg[j]);
#ifdef MACDEBUG
          csound->Message(csound, "defining argument %s ", nn->name);
#endif
          i = 0;
          nn->body = (char*) mmalloc(csound, 100);
          while ((c = getorchar(csound))!= term && c!=trm1) {
            if (UNLIKELY(i > 98)) {
              csound->Die(csound, Str("Missing argument terminator\n%.98s"),
                          nn->body);
            }
            nn->body[i++] = c;
            if (UNLIKELY(i >= size))
              nn->body = mrealloc(csound, nn->body, size += 100);
            if (c == '\n') {
              srccnt++;
            }
          }
          nn->body[i] = '\0';
#ifdef MACDEBUG
          csound->Message(csound, "as...#%s#\n", nn->body);
#endif
          nn->acnt = 0;       /* No arguments for arguments */
          nn->next = ST(macros);
          ST(macros) = nn;
        }
      }
      cp--;                   /* Ignore $ sign */
      ST(input_cnt)++;
      if (ST(input_cnt) >= ST(input_size)) {
        ST(input_size) += 20;
        ST(inputs) = (IN_STACK*) mrealloc(csound, ST(inputs),
                                          ST(input_size) * sizeof(IN_STACK));
      }
      ST(str) = (IN_STACK*) ST(inputs) + (int) ST(input_cnt);
      ST(str)->string = 1; ST(str)->body = mm->body; ST(str)->args = mm->acnt;
      ST(str)->mac = mm;
      ST(str)->line = 1;
      ST(str)->unget_cnt = 0;
      ST(ingappop) = 1;
    }
  }
  if (UNLIKELY(ST(ifdefStack) != NULL))
    lexerr(csound, Str("Unmatched #ifdef"));
  if (UNLIKELY(cp >= endspace)) {                   /* Ought to extend */
    csoundDie(csound, Str("file too large for ortext space"));
  }
  if (*(cp-1) != '\n')                    /* if no final NL,      */
    *cp++ = '\n';                         /*    add one           */
  else --lincnt;
  ST(linadr)[lincnt+1] = NULL;            /* terminate the adrs list */
#ifdef BETA
  csound->Message(csound,Str("%d (%d) lines read\n"),lincnt, srccnt);
#endif
  if (ST(fd) != NULL) {
    csound->FileClose(csound, ST(fd));    /* close the file       */
    ST(fd) = NULL;
  }
  ST(curline) = 0;                        /*   & reset to line 1  */
  ST(ortext) = ortext;
  while (ST(macros)) {                    /* Clear all macros */
    int i;
    mfree(csound, ST(macros)->body);
    mfree(csound, ST(macros)->name);
    for (i = 0; i < ST(macros)->acnt; i++)
      mfree(csound, ST(macros)->arg[i]);
    ST(macros) = ST(macros)->next;
  }                                       /* nullist is a count only */
  ST(nullist) = (ARGLST *) mmalloc(csound, sizeof(ARGLST));
  ST(nullist)->count = 0;
  ST(nxtarglist) = (ARGLST*) mmalloc(csound, sizeof(ARGLST)
                                     + 200 * sizeof(char*));
}

static void extend_collectbuf(CSOUND *csound, char **cp, int grpcnt)
{
  char  *nn;
  int   i;

  i = (int) ST(lenmax);
  ST(lenmax) <<= 1;
  nn = mrealloc(csound, ST(collectbuf), ST(lenmax) + 16);
  (*cp) += (nn - ST(collectbuf));     /* Adjust pointer */
  for ( ; i < (int) ST(lenmax); i++)
    nn[i] = (char) 0;
  /* Need to correct grp vector */
  for (i = 0; i < grpcnt; i++)
    ST(group)[i] += (nn - ST(collectbuf));
  ST(collectbuf) = nn;
}

static void extend_group(CSOUND *csound)
{
  int32  i, j;

  i = ST(grpmax);
  j = i + (int32) GRPMAX;
  ST(grpmax) = (j++);
  ST(group) = (char **) mrealloc(csound, ST(group), j * sizeof(char *));
  ST(grpsav) = (char **) mrealloc(csound, ST(grpsav), j * sizeof(char *));
  while (++i < j) {
    ST(group)[i] = (char *) NULL;
    ST(grpsav)[i] = (char *) NULL;
  }
}

/* split next orch line into atomic groups, count */
/*  labels this line, and set opgrpno where found */

static int splitline(CSOUND *csound)
{
  int     grpcnt, prvif, prvelsif, logical, condassgn, parens;
  int     c, collecting;
  char    *cp, *lp, *grpp = NULL;

  if (ST(collectbuf) == NULL)
    ST(collectbuf) = mcalloc(csound, ST(lenmax) + 16);
 nxtlin:
  if ((lp = ST(linadr)[++ST(curline)]) == NULL)   /* point at next line   */
    return 0;
  csound->DebugMsg(csound, Str("LINE %d:"), CURLINE);
  ST(linlabels) = ST(opgrpno) = 0;
  grpcnt = prvif = prvelsif = logical = condassgn = parens = collecting = 0;
  cp = ST(collectbuf);
  while ((c = *lp++) != '\n') {       /* for all chars this line:   */
    if (cp - ST(collectbuf) >= ST(lenmax))
      extend_collectbuf(csound, &cp, grpcnt);
    if (c == ' ' || c == '\t' || c == '(') {      /* spaces, tabs, (:   */
      if (!ST(opgrpno) && collecting) {           /*  those before args */
        *cp++ = '\0';                             /*  can be delimiters */
        collecting = 0;
        if (strcmp(grpp, "if") == 0) {            /*  of if opcod, */
          strcpy(grpp, "cggoto");                 /*  (replace) */
          cp = grpp + 7;
          prvif++;
        }
        else if (strcmp(grpp, "elseif") == 0) {   /*  of elseif opcod, ... */
          /* check to see we had an 'if' before */
          if (!ST(iflabels)) {
            synterr(csound, Str("invalid 'elseif' statement.  "
                                "must have a corresponding 'if'"));
            goto nxtlin;
          }
          /* check to see we did not have an 'else' before */
          if (UNLIKELY(!ST(iflabels)->els[0])) {
            synterr(csound,
                    Str("'elseif' statement cannot occur after an 'else'"));
            goto nxtlin;
          }
          /* 'elseif' requires 2 additional lines */
          if (ST(repeatingElseifLine)) {
            /* add the 'elselabel' */
            ST(linlabels)++;
            strcpy(grpp, ST(iflabels)->els);
            cp = grpp + strlen(ST(iflabels)->els) + 1;
            /* finally replace the 'elseif' with a 'goto' */
            grpp = ST(group)[grpcnt++] = cp;
            strcpy(grpp, "cggoto");
            cp = grpp + 7;
            prvif++;
            prvelsif++;
            ST(repeatingElseifLine) = 0;
          }
          else {
            /* first add a 'goto endif' for the previous if */
            if (ST(iflabels)->ithen > 0)
              strcpy(grpp, "goto");
            else
              strcpy(grpp, "kgoto");
            if (isopcod(csound, grpp))
              ST(opgrpno) = grpcnt;
            ST(group)[grpcnt] = strchr(grpp, '\0') + 1;
            grpp = ST(group)[grpcnt++];
            strcpy(grpp, ST(iflabels)->end);
            ST(curline)--;    /* roll back one and parse this line again */
            ST(repeatingElseifLine)++;
            ST(linopnum) = ST(opnum);     /* else save full line ops */
            ST(linopcod) = ST(opcod);
            return grpcnt;
          }
        }
        if (isopcod(csound, grpp))                /*  ... or maybe others */
          ST(opgrpno) = grpcnt;
      }
      if (c == ' ' || c == '\t')
        continue;                         /* now discard blanks */
    }
    else if (c == ';') {
      while ((c = *lp++) != '\n');        /* comments:  gobble */
      break;                              /*    & exit linloop */
    }
    else if (c == '/' && *lp == '*') {    /* C Style comments */
      char *ll, *eol;
      ll = strstr(lp++, "*/");
    nxtl:
      eol = strchr(lp, '\n');
      if (eol != NULL && eol < ll) {
        lp = ST(linadr)[++ST(curline)];
        ll = strstr(lp, "*/");
        goto nxtl;
      }
      if (UNLIKELY(ll == NULL)) {
        synterrp(csound, lp - 2, Str("Unmatched comment"));
        lp = eol + 1; break;
      }
      lp = ll + 2;
      continue;
    }
    else if (c == '"') {                  /* quoted string: */
      if (grpcnt >= ST(grpmax))
        extend_group(csound);
      grpp = ST(group)[grpcnt++] = cp;
      *cp++ = c;                          /*  cpy to nxt quote */
      do {
      loop:
        c = *lp++;
        if (c=='\\' && *lp=='"') {        /* Deal with \" case */
          *cp++ = '\\';
          *cp++ = '"';
          lp++;
          goto loop;
        }
        *cp++ = c;
      } while (c != '"' && c != '\n');
      if (c == '\n')
        synterrp(csound, lp - 1, Str("unmatched quotes"));
      collecting = 1;                     /*   & resume chking */
      continue;
    }
    else if (c == '{' && *lp == '{') {    /* multiline quoted string:   */
      if (grpcnt >= ST(grpmax))
        extend_group(csound);
      grpp = ST(group)[grpcnt++] = cp;
      c = '"';                            /*  cpy to nxt quote */
      do {
        *cp++ = c;
        if (cp - ST(collectbuf) >= ST(lenmax))
          extend_collectbuf(csound, &cp, grpcnt);
        c = *(++lp);
        if (c == '\n')
          ++ST(curline);
      } while (!(c == '}' && lp[1] == '}'));
      lp += 2;
      *cp++ = '"';
      collecting = 1;                     /*   & resume chking */
      continue;
    }
    else if (c == ':' && collecting && grpcnt == ST(linlabels)+1) {
      ST(linlabels)++;                    /* colon in 1st grps */
      *cp++ = '\0';                       /*  is also delimitr */
      collecting = 0;                     /*  (do not copy it) */
      continue;
    }
    else if (c == '=' && !ST(opgrpno)) {  /* assign befor args */
      if (collecting)                     /* can be a delimitr */
        *cp++ = '\0';
      grpp = ST(group)[grpcnt++] = cp;    /* is itslf an opcod */
      *cp++ = c;
      *cp++ = '\0';
      isopcod(csound, grpp);
      ST(opgrpno) = grpcnt;
      collecting = 0;                     /* & self-delimiting */
      continue;
    }
    else if (c == ',') {                  /* comma:            */
      if (UNLIKELY(!collecting))
        synterrp(csound, lp - 1, Str("misplaced comma"));
      if (UNLIKELY(parens)) {
        synterrp(csound, lp - 2, Str("unbalanced parens"));
        parens = 0;
      }
      *cp++ = '\0';                       /*  terminate string */
      collecting = logical = condassgn = 0;
      continue;
    }
    if (prvif && collecting && !parens) { /* for prev "if":    */
      if (strncmp(lp-1,"goto",4) == 0) {  /* if found "goto"   */
        *cp++ = '\0';                     /*      delimit cond */
        lp += 3;                          /*      & step over  */
        prvif = collecting = 0;
        continue;
      }
      else if ((c == 'i' || c == 'k') &&          /*  if preced i or k */
               strncmp(lp, "goto", 4) == 0) {     /*  before "goto"    */
        *(ST(group)[ST(opgrpno) - 1] + 1) = c;    /*     modify cggoto */
        isopcod(csound, ST(group)[ST(opgrpno) - 1]);
        *cp++ = '\0';                             /*     then delimit  */
        lp += 4;                                  /*      etc          */
        prvif = collecting = 0;
        continue;
      }
      else if (strncmp(lp - 1, "then", 4) == 0) {
        struct iflabel *prv = ST(iflabels);
        /* modify cggoto */
        *(ST(group)[ST(opgrpno) - 1] + 1) = 'n';
        isopcod(csound, ST(group)[ST(opgrpno) - 1]);
        *cp++ = '\0';
        lp += 3;
        prvif = collecting = 0;
        grpp = ST(group)[grpcnt++] = cp;
        /* synthesize labels to represent an else and endif */
        if (prvelsif) { /* elseif, so we just need a new elselabel */
          sprintf(ST(iflabels)->els, "__else_%d", ST(tempNum)++);
          prvelsif = 0;
        }
        else {
          /* this is a new if, so put a whole new label struct on the stack */
          ST(iflabels) = (struct iflabel *) mmalloc(csound,
                                                    sizeof(struct iflabel));
          ST(iflabels)->prv = prv;
          sprintf(ST(iflabels)->end, "__endif_%d",ST(tempNum)++);
          sprintf(ST(iflabels)->els, "__else_%d", ST(tempNum)++);
        }
        /* we set the 'goto' label to the 'else' label */
        strcpy(grpp, ST(iflabels)->els);
        cp = strchr(grpp, '\0');
        /* set ithen flag to unknown (getoptxt() will update it later) */
        ST(iflabels)->ithen = -1;
        continue;
      }
      else if (strncmp(lp - 1, "ithen", 5) == 0) {
        struct iflabel *prv = ST(iflabels);
        /* modify cggoto */
        *(ST(group)[ST(opgrpno) - 1] + 1) = 'o';
        isopcod(csound, ST(group)[ST(opgrpno) - 1]);
        *cp++ = '\0';
        lp += 4;
        prvif = collecting = 0;
        grpp = ST(group)[grpcnt++] = cp;
        /* synthesize labels to represent an else and endif */
        if (prvelsif) { /* elseif, so we just need a new elselabel */
          sprintf(ST(iflabels)->els, "__else_%d",ST(tempNum)++);
          prvelsif = 0;
        }
        else {
          /* this is a new if, so put a whole new label struct on the stack */
          ST(iflabels) = (struct iflabel *)mmalloc(csound,
                                                   sizeof(struct iflabel));
          ST(iflabels)->prv = prv;
          sprintf(ST(iflabels)->end, "__endif_%d",ST(tempNum)++);
          sprintf(ST(iflabels)->els, "__else_%d", ST(tempNum)++);
        }
        /* we set the 'goto' label to the 'else' label */
        strcpy(grpp, ST(iflabels)->els);
        cp = strchr(grpp, '\0');
        /* set ithen flag */
        ST(iflabels)->ithen = 1;
        continue;
      }
    }
    if (!collecting++) {                  /* remainder are     */
      if (grpcnt >= ST(grpmax))           /* collectable chars */
        extend_group(csound);
      grpp = ST(group)[grpcnt++] = cp;
    }
    *cp++ = c;                            /* collect the char  */
    /* establish validity: allow letters, digits, and underscore */
    /* in label, variable, and opcode names */
    if (isalnum(c) || c == '_')
      continue;
    /* other characters are valid only after an opcode */
    if (UNLIKELY(!ST(opgrpno)))
      goto char_err;
    switch (c) {
    case '<':
    case '>':
      if (*lp == c) {
        lp++; *cp++ = c;                  /* <<, >> */
      }
      else if (prvif || parens)           /* <, <=, >=, > */
        logical++;
      else
        goto char_err;
      break;
    case '&':
    case '|':
      if (*lp == c) {                     /* &&, ||, &, | */
        if (UNLIKELY(!prvif && !parens))
          goto char_err;
        logical++; lp++; *cp++ = c;
      }
      break;
    case '!':
    case '=':
      if (UNLIKELY(!prvif && !parens))              /* ==, !=, <=, >= */
        goto char_err;
      logical++;
      break;
    case '+':                             /* arithmetic and bitwise ops */
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
    case '#':                             /* XOR */
    case '\254':                          /* NOT (same as ~) */
    case '~':
    case '.':
      break;
    case '\302':
      if (*lp == '\254')                  /* NOT operator in UTF-8 format */
        *(cp - 1) = *lp++;
      else
        goto char_err;
      break;
    case '(':
      parens++;                           /* and monitor function */
      break;
    case ')':
      if (UNLIKELY(!parens)) {
        synterrp(csound, lp - 1, Str("unbalanced parens"));
        cp--;
      }
      else
        --parens;
      break;
    case '?':
      if (UNLIKELY(!logical))
        goto char_err;
      condassgn++;
      break;
    case ':':
      if (UNLIKELY(!condassgn))
        goto char_err;
      break;
    default:
      goto char_err;
    }
    continue;                             /* loop back for next character */
  char_err:
    {
      char err_msg[64];
      sprintf(err_msg, Str("illegal character %c"), c);
      synterrp(csound, lp - 1, err_msg);
      cp--;
    }
  }
  *cp = '\0';                             /* terminate last group */
  if (grpp && grpcnt == (ST(linlabels) + 1)) {
    /* convert an 'else' statement into 2 lines
       goto <endiflabel>
       <elselabel>
       to do this, we parse the current twice */
    if (strcmp(grpp, "else") == 0) {
      if (UNLIKELY(!ST(iflabels))) {    /* 'else': check to see we had an 'if' before */
        synterr(csound, Str("invalid 'else' statement.  "
                            "must have a corresponding 'if'"));
        goto nxtlin;
      }
      if (ST(repeatingElseLine)) {        /* add the elselabel */
        if (UNLIKELY(!ST(iflabels)->els[0])) {
          /* check to see we had not another 'else' */
          synterr(csound, Str("duplicate 'else' statement"));
          goto nxtlin;
        }
        ST(linlabels)++;
        strcpy(grpp, ST(iflabels)->els);
        ST(iflabels)->els[0] = '\0';
        ST(repeatingElseLine) = 0;
      }
      else {                              /* add the goto statement */
        if (ST(iflabels)->ithen > 0)
          strcpy(grpp, "goto");
        else
          strcpy(grpp, "kgoto");
        ST(linlabels) = 0;                /* ignore any labels this time */
        ST(group)[0] = grpp;
        grpcnt = 1;
        if (isopcod(csound, grpp))
          ST(opgrpno) = grpcnt;
        ST(group)[grpcnt] = strchr(grpp, '\0') + 1;
        grpp = ST(group)[grpcnt++];
        strcpy(grpp, ST(iflabels)->end);
        ST(curline)--;        /* roll back one and parse this line again */
        ST(repeatingElseLine) = 1;
      }
    }
    else if (strcmp(grpp, "endif") == 0) {
      /* replace 'endif' with the synthesized label */
      struct iflabel *prv;
      if (UNLIKELY(!ST(iflabels))) {    /* check to see we had an 'if' before  */
        synterr(csound, Str("invalid 'endif' statement.  "
                            "must have a corresponding 'if'"));
        goto nxtlin;
      }
      if (ST(iflabels)->els[0]) {
        /* we had no 'else' statement, so we need to insert the elselabel */
        ST(linlabels)++;
        strcpy(grpp, ST(iflabels)->els);
        ST(iflabels)->els[0] = '\0';
        ST(curline)--;        /* roll back one and parse this line again */
      }
      else {
        prv = ST(iflabels)->prv;
        ST(linlabels)++;
        strcpy(grpp, ST(iflabels)->end);
        mfree(csound, ST(iflabels));
        ST(iflabels) = prv;
      }
    }
  }
  if (!grpcnt)                        /* if line was trivial,    */
    goto nxtlin;                      /*      try another        */
  if (collecting && !ST(opgrpno)) {   /* if still collecting,    */
    if (isopcod(csound, grpp))        /*      chk for opcod      */
      ST(opgrpno) = grpcnt;
  }
  if (UNLIKELY(parens))                                   /* check balanced parens   */
    synterrp(csound, lp - 1, Str("unbalanced parens"));
  if (UNLIKELY(grpcnt > ST(linlabels) && !ST(opgrpno))) { /* if no full line opcod,  */
    synterr(csound, Str("no legal opcode"));    /*      complain &         */
    goto nxtlin;                                /*      try another        */
  }
  ST(linopnum) = ST(opnum);                     /* else save full line ops */
  ST(linopcod) = ST(opcod);
  if (UNLIKELY(csound->oparms->odebug))
    printgroups(csound, grpcnt);
  return grpcnt;
}

static void resetouts(CSOUND *csound)
{
  csound->acount = csound->kcount = csound->icount = 0;
  csound->Bcount = csound->bcount = 0;
}

TEXT *getoptxt(CSOUND *csound, int *init)
{                               /* get opcod and args from current line */
                                /*      returns pntr to a TEXT struct   */
  TEXT        *tp;
  char        c, d, str[64], *s;
  int         nn, incnt, outcnt;

  if (*init) {
    ST(grpcnt)   = 0;
    ST(nxtest)   = 1;
    ST(xprtstno) = 0;
    ST(polcnt)   = 0;
    ST(instrblk) = 0;
    ST(opcodblk) = 0;     /* IV - Sep 8 2002 */
    ST(instrcnt) = 0;
    *init    = 0;
    memset(&ST(optext), 0, sizeof(TEXT));
  }

 tstnxt:
  tp = &ST(optext);
  if (ST(nxtest) >= ST(grpcnt)) {             /* if done with prevline, */
    csound->argcnt_offs = 0;          /* reset temporary variable index */
    if (!(ST(grpcnt) = splitline(csound))) {  /*    attack next line    */
      /* end of orchestra, clean up */
      mfree(csound, ST(linadr));      ST(linadr) = NULL;
      mfree(csound, ST(ortext));      ST(ortext) = NULL;
      mfree(csound, ST(collectbuf));  ST(collectbuf) = NULL;
      mfree(csound, ST(group));       ST(group) = NULL;
      mfree(csound, ST(grpsav));      ST(grpsav) = NULL;
      mfree(csound, csound->tokens);      csound->tokens = NULL;
      mfree(csound, csound->tokenlist);   csound->tokenlist = NULL;
      mfree(csound, csound->tokenstring); csound->tokenstring = NULL;
      mfree(csound, csound->polish);      csound->polish = NULL;
      csound->token = NULL;
      return (TEXT*) NULL;                    /*    (else we're done)   */
    }
    for (nn=0; nn<ST(grpcnt); nn++)           /*    save the group pntrs */
      ST(grpsav)[nn] = ST(group)[nn];
    ST(xprtstno) = ST(grpcnt) - 1;            /*    and reinit indices  */
    ST(nxtest) = 0;
    tp->linenum = ST(curline);
    /* IV - Jan 27 2005 */
    if (csound->oparms->expr_opt) {
      int i = (int) ST(linlabels) + 1;
      if (((int) ST(grpcnt) - i) > 0 && ST(group)[i][0] == '=' &&
          ST(group)[i][1] == '\0') {
        /* if opcode is '=', save outarg and type for expression optimiser */
        csound->opcode_is_assign = 1;
        csound->assign_type = (int) argtyp(csound, ST(group)[ST(linlabels)]);
        csound->assign_outarg = strsav_string(csound,
                                              ST(group)[ST(linlabels)]);
      }
      else {
        csound->opcode_is_assign = csound->assign_type = 0;
        csound->assign_outarg = NULL;
      }
    }
  }
  if (ST(linlabels)) {
    s = strsav_string(csound, ST(group)[ST(nxtest)]);
    lblfound(csound, s);
    tp->opnum = LABEL;
    tp->opcod = s;
    tp->inlist = tp->outlist = ST(nullist);
    ST(linlabels)--;
    ST(nxtest)++;
    return(tp);
  }
  if (!ST(instrcnt)) {                          /* send initial "instr 0"  */
    tp->opnum = INSTR;
    tp->opcod = strsav_string(csound, "instr"); /*  to hold global assigns */
    tp->outlist = ST(nullist);
    ST(nxtarglist)->count = 1;
    ST(nxtarglist)->arg[0] = strsav_string(csound, "0");
    tp->inlist = copy_arglist(csound, ST(nxtarglist));
    ST(instrcnt) = ST(instrblk) = 1;
    return(tp);
  }                                             /* then at 1st real INSTR, */
  /*               or OPCODE, */
  if (ST(instrcnt) == 1 && ST(instrblk) &&
      (ST(opnum) == INSTR || ST(opnum) == OPCODE)) {
    tp->opnum = ENDIN;                          /*  send an endin to */
    tp->opcod = strsav_string(csound, "endin"); /*  term instr 0 blk */
    tp->outlist = tp->inlist = ST(nullist);
    ST(instrblk) = 0;
    ST(instrcnt) = 2;
    return(tp);
  }
  while (ST(xprtstno) >= 0) {             /* for each arg (last 1st):  */
    if (!ST(polcnt)) {
      /* if not midst of expressn: tst nxtarg */
      ST(polcnt) = express(csound, ST(group)[ST(xprtstno)--]);
      /* IV - Feb 06 2006: if there is an if/then with an unknown rate: */
      if (ST(polcnt) > 0 && ST(iflabels) != NULL && ST(iflabels)->ithen < 0) {
        char  tmp;
        /* check the output type of the expression (FIXME: is this safe ?) */
        /* if it is an i-rate conditional, set ithen flag for else/elseif */
        tmp = argtyp(csound, csound->tokenlist[0]->str);
        if (tmp == (char) 'b')
          ST(iflabels)->ithen = 1;
        else
          ST(iflabels)->ithen = 0;
      }
    }
    if (ST(polcnt) < 0) {
      /* polish but arg only: redo ptr & contin */
      ST(group)[ST(xprtstno)+1] = strsav_string(csound, csound->tokenstring);
      ST(polcnt) = 0;
    }
    else if (ST(polcnt)) {
      POLISH  *pol;                           /* for real polish ops, */
      int n;
      pol = &(csound->polish[--ST(polcnt)]);  /*    grab top one      */
      if (UNLIKELY(isopcod(csound, pol->opcod) == 0)) { /* and check it out     */
        synterr(csound, Str("illegal opcod from expr anal"));
        goto tstnxt;
      }
      tp->opnum = ST(opnum);                  /* ok to send subop     */
      tp->opcod = strsav_string(csound, ST(opcod));
      ST(nxtarglist)->count = outcnt = 1;
      ST(nxtarglist)->arg[0] = strsav_string(csound, pol->arg[0]);
      tp->outlist = copy_arglist(csound, ST(nxtarglist));
      n = ST(nxtarglist)->count = incnt = pol->incount;
      do  ST(nxtarglist)->arg[n-1] = strsav_string(csound, pol->arg[n]);
      while (--n);
      tp->inlist = copy_arglist(csound, ST(nxtarglist));
      if (!ST(polcnt))                    /* last op? hit the grp ptr */
        ST(group)[ST(xprtstno)+1] = tp->outlist->arg[0];
      goto spctst;
    }
  }
  if (!strcmp(ST(linopcod), "=")) {       /* IV - Jan 08 2003: '=' opcode */
    if (csound->oparms->expr_opt && csound->opcode_is_assign < 0) {
      /* if optimised away, skip line */
      ST(nxtest) = ST(grpcnt); goto tstnxt;
    }
    if (ST(nxtest) < ST(opgrpno)) {
      c = argtyp(csound, ST(group)[ST(nxtest)]);
      switch (c) {
      case 'S': strcpy(str, "strcpy"); break;
      case 'a': c = argtyp(csound, ST(group)[ST(opgrpno)]);
        strcpy(str, (c == 'a' ? "=.a" : "upsamp")); break;
      case 'p': c = 'i';
      default:  sprintf(str, "=.%c", c);
      }
      if (UNLIKELY(!(isopcod(csound, str)))) {
        synterr(csound,
                Str("failed to find %s, output arg '%s' illegal type"),
                str, ST(group)[ST(nxtest)]);  /* report syntax error     */
        ST(nxtest) = 100;                     /* step way over this line */
        goto tstnxt;                          /* & go to next            */
      }
      if (strcmp(ST(group)[ST(nxtest)], ST(group)[ST(opgrpno)]) == 0) {
        /* outarg same as inarg, skip line */
        ST(nxtest) = ST(grpcnt); goto tstnxt;
      }
      ST(linopnum) = ST(opnum);
      ST(linopcod) = ST(opcod);
      csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
    }
  }
  else if (ST(nxtest) < ST(opgrpno) &&  /* Some aopcodes do not have ans! */
           csound->opcodlst[ST(linopnum)].dsblksiz == 0xffff) {
    /* use outype to modify some opcodes flagged as translating */
    c = argtyp(csound, ST(group)[ST(nxtest)]);
    if (c == 'p')   c = 'i';
    if (c == '?')   c = 'a';                  /* tmp */
    sprintf(str, "%s.%c", ST(linopcod), c);
    if (UNLIKELY(!(isopcod(csound, str)))) {
      synterr(csound, Str("failed to find %s, output arg '%s' illegal type"),
              str, ST(group)[ST(nxtest)]);    /* report syntax error     */
      ST(nxtest) = 100;                       /* step way over this line */
      goto tstnxt;                            /* & go to next            */
    }
    ST(linopnum) = ST(opnum);
    ST(linopcod) = ST(opcod);
    csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
  }
  else if ((int) csound->opcodlst[ST(linopnum)].dsblksiz >= 0xfffb) {
    c = argtyp(csound, ST(group)[ST(opgrpno)]); /* type of first input arg */
    switch ((int) csound->opcodlst[ST(linopnum)].dsblksiz) {
    case 0xfffe:                              /* Two tags for OSCIL's    */
      if (c != 'a') c = 'k';
      if ((d = argtyp(csound, ST(group)[ST(opgrpno)+1])) != 'a') d = 'k';
      sprintf(str, "%s.%c%c", ST(linopcod), c, d);
      break;
    case 0xfffd:                              /* For peak, etc.          */
      if (c != 'a') c = 'k';
      sprintf(str, "%s.%c", ST(linopcod), c);
      break;
    case 0xfffc:                              /* For divz types          */
      d = argtyp(csound, ST(group)[ST(opgrpno)+1]);
      if ((c=='i' || c=='c') && (d=='i' || d=='c'))
        c = 'i', d = 'i';
      else {
        if (c != 'a') c = 'k';
        if (d != 'a') d = 'k';
      }
      sprintf(str, "%s.%c%c", ST(linopcod), c, d);
      break;
    case 0xfffb:          /* determine opcode by type of first input arg */
      /* allows a, k, and i types (e.g. Inc, Dec), but not constants */
      if (ST(typemask_tabl)[(unsigned char) c] & (ARGTYP_i | ARGTYP_p))
        c = 'i';
      sprintf(str, "%s.%c", ST(linopcod), c);
      break;
    default:
      strcpy(str, ST(linopcod));  /* unknown code: use original opcode   */
    }
    if (UNLIKELY(!(isopcod(csound, str)))) {
      /* if opcode is not found: report syntax error     */
      synterr(csound, Str("failed to find %s, input arg illegal type"), str);
      ST(nxtest) = 100;                       /* step way over this line */
      goto tstnxt;                            /* & go to next            */
    }
    ST(linopnum) = ST(opnum);
    ST(linopcod) = ST(opcod);
    csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
  }
  tp->opnum = ST(linopnum);                         /* now use identified   */
  tp->opcod = strsav_string(csound, ST(linopcod));  /*   full line opcode   */
  /* IV - Oct 24 2002: check for invalid use of setksmps */
  if (strcmp(ST(linopcod), "setksmps") == 0) {
    if (UNLIKELY(!ST(opcodblk)))
      synterr(csound,
              Str("setksmps is allowed only in user defined opcodes"));
    else if (UNLIKELY((int) ST(opcodflg) & 4))
      synterr(csound,
              Str("multiple uses of setksmps in the same opcode definition"));
    else
      ST(opcodflg) |= (int16) 4;
  }
#if 0
  /* NO LONGER USED */
  if (strncmp(ST(linopcod),"out",3) == 0 && /* but take case of MIDI ops */
      (ST(linopcod)[3] == '\0' || ST(linopcod)[3] == 's' ||
       ST(linopcod)[3] == 'q'  || ST(linopcod)[3] == 'h' ||
       ST(linopcod)[3] == 'o'  || ST(linopcod)[3] == 'x' ||
       ST(linopcod)[3] == '3'     ))
    if ((csound->tran_nchnls == 1  && strcmp(ST(linopcod),"out" ) != 0)    ||
        (csound->tran_nchnls == 2  && strncmp(ST(linopcod),"outs",4) != 0) ||
        (csound->tran_nchnls == 4  && strncmp(ST(linopcod),"outq",4) != 0) ||
        (csound->tran_nchnls == 6  && strncmp(ST(linopcod),"outh",4) != 0) ||
        (csound->tran_nchnls == 8  && strncmp(ST(linopcod),"outo",4) != 0) ||
        (csound->tran_nchnls == 16 && strncmp(ST(linopcod),"outx",4) != 0) ||
        (csound->tran_nchnls == 32 && strncmp(ST(linopcod),"out32",5) != 0)) {
      if      (csound->tran_nchnls == 1)  isopcod(csound, "out");
      else if (csound->tran_nchnls == 2)  isopcod(csound, "outs");
      else if (csound->tran_nchnls == 4)  isopcod(csound, "outq");
      else if (csound->tran_nchnls == 6)  isopcod(csound, "outh");
      else if (csound->tran_nchnls == 8)  isopcod(csound, "outo");
      else if (csound->tran_nchnls == 16) isopcod(csound, "outx");
      else if (csound->tran_nchnls == 32) isopcod(csound, "out32");
      csound->Message(csound, Str("%s inconsistent with global nchnls (%d); "
                                  "replaced with %s\n"),
                      ST(linopcod), csound->tran_nchnls, ST(opcod));
      tp->opnum = ST(linopnum) = ST(opnum);
      tp->opcod = strsav_string(csound, ST(linopcod) = ST(opcod));
    }
#endif
  incnt = outcnt = 0;
  while (ST(nxtest) < ST(opgrpno)-1)          /* create the out arglist  */
    ST(nxtarglist)->arg[outcnt++] =
      strsav_string(csound, ST(group)[ST(nxtest)++]);
  ST(nxtarglist)->count = outcnt;
  if (outcnt == 0)
    tp->outlist = ST(nullist);
  else {
    tp->outlist = copy_arglist(csound, ST(nxtarglist));   /* & prep ins */
  }
  ST(nxtest)++;
  while (ST(nxtest) < ST(grpcnt))             /*      & ensuing inargs  */
    ST(nxtarglist)->arg[incnt++] =
      strsav_string(csound, ST(group)[ST(nxtest)++]);
  ST(nxtarglist)->count = incnt;
  if (incnt==0)
    tp->inlist = ST(nullist);
  else tp->inlist = copy_arglist(csound, ST(nxtarglist));
  ST(grpcnt) = 0;                             /* all done w. these groups */

 spctst:
  tp->xincod_str = tp->xincod = 0;
  if (tp->opnum == OPCODE) {  /* IV - Sep 8 2002: added OPCODE and ENDOP */
    if (UNLIKELY(ST(opcodblk)))
      synterr(csound, Str("opcode blks cannot be nested (missing 'endop'?)"));
    else if (UNLIKELY(ST(instrblk)))
      synterr(csound, Str("opcode not allowed in instr block"));
    else ST(instrblk) = ST(opcodblk) = 1;
    ST(opcodflg) = 0;
    resetouts(csound);                        /* reset #out counts */
    lblclear(csound);                         /* restart labelist  */
  }
  else if (tp->opnum == ENDOP) {      /* IV - Sep 8 2002:     ENDOP:  */
    lblchk(csound);                   /* chk missed labels */
    if (UNLIKELY(!ST(instrblk)))
      synterr(csound, Str("unmatched endop"));
    else if (UNLIKELY(!ST(opcodblk)))
      synterr(csound, Str("endop not allowed in instr block"));
    else ST(instrblk) = ST(opcodblk) = 0;
  }
  else if (tp->opnum == INSTR) {      /* IV - Sep 8 2002: for opcod INSTR  */
    if (UNLIKELY(ST(opcodblk)))     /* IV - Sep 8 2002 */
      synterr(csound, Str("instr not allowed in opcode block"));
    else if (UNLIKELY(ST(instrblk)))
      synterr(csound,
              Str("instr blocks cannot be nested (missing 'endin'?)"));
    else ST(instrblk) = 1;
    resetouts(csound);                        /* reset #out counts */
    lblclear(csound);                         /* restart labelist  */
  }
  else if (tp->opnum == ENDIN) {              /* ENDIN:       */
    lblchk(csound);                           /* chk missed labels */
    if (UNLIKELY(ST(opcodblk)))
      synterr(csound, Str("endin not allowed in opcode blk"));
    else if (UNLIKELY(!ST(instrblk)))
      synterr(csound, Str("unmatched endin"));
    else ST(instrblk) = 0;
  }
  else {                                      /* for all other opcodes:  */
    OENTRY    *ep = csound->opcodlst + tp->opnum;
    int       n, nreqd;
    char      tfound = '\0', treqd, *types = NULL;
    char      xtypes[OPCODENUMOUTS_MAX + 1];  /* IV - Oct 24 2002 */

    if (UNLIKELY(!ST(instrblk)))
      synterr(csound, Str("misplaced opcode"));
    /* IV - Oct 24 2002: moved argument parsing for xout here */
    n = incnt;
    nreqd = -1;
    if (!strcmp(ep->opname, "xout")) {
      if (UNLIKELY(!ST(opcodblk)))
        synterr(csound, Str("xout is allowed only in user defined opcodes"));
      else if (UNLIKELY((int) ST(opcodflg) & 2))
        synterr(csound,
                Str("multiple uses of xout in the same opcode definition"));
      else {
        /* IV - Oct 24 2002: opcodeInfo always points to the most recently */
        /* defined user opcode (or named instrument) structure; in this */
        /* case, it is the current opcode definition (not very elegant, */
        /* but works) */
        char *c = csound->opcodeInfo->outtypes;
        int i = 0;
        ST(opcodflg) |= (int16) 2;
        nreqd = csound->opcodeInfo->outchns;
        /* replace opcode if needed */
        if (nreqd > OPCODENUMOUTS_LOW) {
          if (nreqd > OPCODENUMOUTS_HIGH)
            isopcod(csound, ".xout256");
          else
            isopcod(csound, ".xout64");
          ST(linopcod) = ST(opcod);
          ST(linopnum) = ST(opnum);
          tp->opcod = strsav_string(csound, ST(linopcod));
          tp->opnum = ST(linopnum);
          ep = csound->opcodlst + tp->opnum;
          csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
        }
        while (c[i]) {
          switch (c[i]) {
          case 'a':
          case 'k':
          case 'f':
          case 'i': xtypes[i] = c[i]; break;
          case 'K': xtypes[i] = 'k';
          }
          i++;
        }
        xtypes[i] = '\0';
        types = &xtypes[0];
      }
    }
    if (nreqd < 0)    /* for other opcodes */
      nreqd = strlen(types = ep->intypes);
    if (n > nreqd) {                  /* IV - Oct 24 2002: end of new code */
      if ((treqd = types[nreqd-1]) == 'n') {  /* indef args: */
        if (UNLIKELY(!(incnt & 01)))                    /* require odd */
          synterr(csound, Str("missing or extra arg"));
      }       /* IV - Sep 1 2002: added 'M' */
      else if (UNLIKELY(treqd != 'm' && treqd != 'z' && treqd != 'y' &&
                        treqd != 'Z' && treqd != 'M' &&
                        treqd != 'N')) /* else any no */
        synterr(csound, Str("too many input args"));
    }
    else if (incnt < nreqd) {         /*  or set defaults: */
      do {
        switch (types[incnt]) {
        case 'O':             /* Will this work?  Doubtful code.... */
        case 'o': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "0");
          break;
        case 'P':
        case 'p': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "1");
          break;
        case 'q': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "10");
          break;
        case 'V':
        case 'v': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, ".5");
          break;
        case 'h': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "127");
          break;
        case 'J':
        case 'j': ST(nxtarglist)->arg[incnt++] = strsav_string(csound, "-1");
          break;
        case 'F':
        case 'M':
        case 'N':
        case 'm': nreqd--;
          break;
        default:  synterr(csound, Str("insufficient required arguments"));
          goto chkin;
        }
      } while (incnt < nreqd);
      ST(nxtarglist)->count = n = incnt;          /*    in extra space */
      if (tp->inlist == ST(nullist) && incnt > 0) {
        /*MWB 2/11/97 fixed bug that prevented an
          opcode with only optional arguments from
          properly loading defaults */
        tp->inlist = copy_arglist(csound, ST(nxtarglist));
      }
    }
  chkin:
    if (n>tp->inlist->count) {
      int i;
      size_t m = sizeof(ARGLST) + (n - 1) * sizeof(char*);
      tp->inlist = (ARGLST*) mrealloc(csound, tp->inlist, m);
      for (i=tp->inlist->count; i<n; i++) {
        tp->inlist->arg[i] = ST(nxtarglist)->arg[i];
      }
      tp->inlist->count = n;
    }
    while (n--) {                     /* inargs:   */
      int32    tfound_m, treqd_m = 0L;
      s = tp->inlist->arg[n];
      if (n >= nreqd) {               /* det type required */
        switch (types[nreqd-1]) {
        case 'M':
        case 'N':
        case 'Z':
        case 'y':
        case 'z':   treqd = types[nreqd-1]; break;
        default:    treqd = 'i';    /*   (indef in-type) */
        }
      }
      else treqd = types[n];          /*       or given)   */
      if (treqd == 'l') {             /* if arg takes lbl  */
        csound->DebugMsg(csound, "treqd = l");
        lblrequest(csound, s);        /*      req a search */
        continue;                     /*      chk it later */
      }
      tfound = argtyp(csound, s);     /* else get arg type */
      /* IV - Oct 31 2002 */

      tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
      if (UNLIKELY(!(tfound_m & (ARGTYP_c|ARGTYP_p)) &&
                   !ST(lgprevdef) && *s != '"')) {
        synterr(csound, Str("input arg '%s' used before defined \n"), s);
      }
      csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
      if (tfound == 'a' && n < 31)    /* JMC added for FOG */
        /* 4 for FOF, 8 for FOG; expanded to 15  */
        tp->xincod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xincod_str |= (1 << n);
      /* IV - Oct 31 2002: simplified code */
      if (!(tfound_m & ST(typemask_tabl_in)[(unsigned char) treqd])) {
        /* check for exceptional types */
        switch (treqd) {
        case 'I':
          treqd_m = ARGTYP_i;
          break;
        case 'Z':                             /* indef kakaka ... */
          if (UNLIKELY(!(tfound_m & (n & 1 ? ARGTYP_a : ARGTYP_ipcrk))))
            intyperr(csound, n, tfound, treqd);
          break;
        case 'x':
          treqd_m = ARGTYP_ipcr;              /* also allows i-rate */
        case 's':                             /* a- or k-rate */
          treqd_m |= ARGTYP_a | ARGTYP_k;
          if (tfound_m & treqd_m) {
            if (tfound == 'a' && tp->outlist != ST(nullist)) {
              int32 outyp_m =                  /* ??? */
                ST(typemask_tabl)[(unsigned char) argtyp(csound,
                                                         tp->outlist->arg[0])];
              if (outyp_m & (ARGTYP_a | ARGTYP_w | ARGTYP_f)) break;
            }
            else
              break;
          }
        default:
          intyperr(csound, n, tfound, treqd);
          break;
        }
      }
    }
    csound->DebugMsg(csound, "xincod = %d", tp->xincod);
    /* IV - Sep 1 2002: added 'X' type, and xoutcod */
    tp->xoutcod_str = tp->xoutcod = 0;
    /* IV - Oct 24 2002: moved argument parsing for xin here */
    n = outcnt;
    nreqd = -1;
    if (!strcmp(ep->opname, "xin")) {
      if (UNLIKELY(!ST(opcodblk)))
        synterr(csound, Str("xin is allowed only in user defined opcodes"));
      else if (UNLIKELY((int) ST(opcodflg) & 1))
        synterr(csound,
                Str("multiple uses of xin in the same opcode definition"));
      else {
        /* IV - Oct 24 2002: opcodeInfo always points to the most recently */
        /* defined user opcode (or named instrument) structure; in this */
        /* case, it is the current opcode definition (not very elegant, */
        /* but works) */
        char *c = csound->opcodeInfo->intypes;
        int i = 0;
        ST(opcodflg) |= (int16) 1;
        nreqd = csound->opcodeInfo->inchns;
        /* replace opcode if needed */
        if (nreqd > OPCODENUMOUTS_LOW) {
          if (nreqd > OPCODENUMOUTS_HIGH)
            isopcod(csound, ".xin256");
          else
            isopcod(csound, ".xin64");
          ST(linopcod) = ST(opcod);
          ST(linopnum) = ST(opnum);
          tp->opcod = strsav_string(csound, ST(linopcod));
          tp->opnum = ST(linopnum);
          ep = csound->opcodlst + tp->opnum;
          csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));
        }
        while (c[i]) {
          switch (c[i]) {
          case 'a': xtypes[i] = c[i]; break;
          case  'f': xtypes[i] = c[i]; break;
          case 'k':
          case 'P':
          case 'K': xtypes[i] = 'k'; break;
          case 'S': xtypes[i] = 'S'; break;
          default:  xtypes[i] = 'i';
          }
          i++;
        }
        xtypes[i] = '\0';
        types = &xtypes[0];
      }
    }
    if (nreqd < 0)    /* for other opcodes */
      nreqd = strlen(types = ep->outypes);
    if (UNLIKELY((n != nreqd) &&      /* IV - Oct 24 2002: end of new code */
                 !(n > 0 && n < nreqd &&
                   (types[n] == 'm' || types[n] == 'z' || types[n] == 'I' ||
                    types[n] == 'X' || types[n] == 'N' || types[n] == 'F')))) {
      synterr(csound, Str("illegal no of output args"));
      if (n > nreqd)
        n = nreqd;
    }
    while (n--) {                                     /* outargs:  */
      int32    tfound_m;       /* IV - Oct 31 2002 */
      s = tp->outlist->arg[n];
      treqd = types[n];
      tfound = argtyp(csound, s);                     /*  found    */
      /* IV - Oct 31 2002 */
      tfound_m = ST(typemask_tabl)[(unsigned char) tfound];
      /* IV - Sep 1 2002: xoutcod is the same as xincod for input */
      if (tfound == 'a' && n < 31)
        tp->xoutcod |= (1 << n);
      if (tfound == 'S' && n < 31)
        tp->xoutcod_str |= (1 << n);
      csound->DebugMsg(csound, "treqd %c, tfound %c", treqd, tfound);
      if (tfound_m & ARGTYP_w)
        if (UNLIKELY(ST(lgprevdef))) {
          synterr(csound, Str("output name previously used, "
                              "type '%c' must be uniquely defined"), tfound);
        }
      /* IV - Oct 31 2002: simplified code */
      if (UNLIKELY(!(tfound_m & ST(typemask_tabl_out)[(unsigned char) treqd]))) {
        synterr(csound, Str("output arg '%s' illegal type"), s);
      }
    }
    if (incnt) {
      if (ep->intypes[0] != 'l')      /* intype defined by 1st inarg */
        tp->intype = argtyp(csound, tp->inlist->arg[0]);
      else tp->intype = 'l';          /*   (unless label)  */
    }
    if (outcnt)                       /* pftype defined by outarg */
      tp->pftype = tfound;
    else tp->pftype = tp->intype;     /*    else by 1st inarg     */
  }
  return(tp);                         /* return the text blk */
}

static void intyperr(CSOUND *csound, int n, char tfound, char expect)
{
  char    *s = ST(grpsav)[ST(opgrpno) + n];
  char    t[10];

  switch (tfound) {
  case 'w':
  case 'f':
  case 'a':
  case 'k':
  case 'i':
  case 'P':
  case 'p': t[0] = tfound;
    t[1] = '\0';
    break;
  case 'r':
  case 'c': strcpy(t,"const");
    break;
  case 'S': strcpy(t,"string");
    break;
  case 'b':
  case 'B': strcpy(t,"boolean");
    break;
  case '?': strcpy(t,"?");
    break;
  }
  synterr(csound, Str("input arg '%s' of type %s "
                      "not allowed when expecting %c"), s, t, expect);
}

static int isopcod(CSOUND *csound, char *s)
{                               /* tst a string against opcodlst  */
  int     n;                  /*   & set op carriers if matched */

  if (!(n = find_opcode(csound, s))) return (0);      /* IV - Oct 31 2002 */
  ST(opnum) = n;                          /* on corr match,   */
  ST(opcod) = csound->opcodlst[n].opname; /*  set op carriers */

  return(1);                              /*  & report success */
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

static void lblclear(CSOUND *csound)
{
  ST(lblcnt) = 0;
}

static void lblrequest(CSOUND *csound, char *s)
{
  int     req;

  for (req=0; req<ST(lblcnt); req++)
    if (strcmp(ST(lblreq)[req].label,s) == 0)
      return;
  if (++ST(lblcnt) >= ST(lblmax)) {
    LBLREQ *tmp;
    ST(lblmax) += LBLMAX;
    tmp = mrealloc(csound, ST(lblreq), ST(lblmax) * sizeof(LBLREQ));
    ST(lblreq) = tmp;
  }
  ST(lblreq)[req].reqline = ST(curline);
  ST(lblreq)[req].label =s;
}

static void lblfound(CSOUND *csound, char *s)
{
  int     req;

  for (req=0; req<ST(lblcnt); req++ )
    if (strcmp(ST(lblreq)[req].label,s) == 0) {
      if (UNLIKELY(ST(lblreq)[req].reqline == 0))
        synterr(csound, Str("duplicate label"));
      goto noprob;
    }
  if (++ST(lblcnt) >= ST(lblmax)) {
    LBLREQ *tmp;
    ST(lblmax) += LBLMAX;
    tmp = mrealloc(csound, ST(lblreq), ST(lblmax) * sizeof(LBLREQ));
    ST(lblreq) = tmp;
  }
  ST(lblreq)[req].label = s;
 noprob:
  ST(lblreq)[req].reqline = 0;
}

static void lblchk(CSOUND *csound)
{
  int req;
  int n;

  for (req=0; req<ST(lblcnt); req++ )
    if (UNLIKELY((n = ST(lblreq)[req].reqline))) {
      char    *s;
      csound->Message(csound, Str("error line %d.  unknown label:\n"), n);
      s = ST(linadr)[n];
      do {
        csound->Message(csound, "%c", *s);
      } while (*s++ != '\n');
      csound->synterrcnt++;
    }
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
#ifdef never
  if (ST(linadr) != NULL && (cp = ST(linadr)[ST(curline)]) != NULL
#if defined(ENABLE_NEW_PARSER)
      && !csound->oparms->newParser
#endif
      ) {
    csound->MessageS(csound, CSOUNDMSG_ERROR,
                     Str(", line %d:\n"), CURLINE);
    do {
      csound->MessageS(csound, CSOUNDMSG_ERROR, "%c", (c = *cp++));
    } while (c != '\n');
  }
  else {
    csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }
#endif
  csound->synterrcnt++;
}

static void synterrp(CSOUND *csound, const char *errp, char *s)
{
  char    *cp;

  synterr(csound, s);
  cp = ST(linadr)[ST(curline)];
  while (cp < errp) {
    int ch = *cp++;
    if (ch != '\t') ch = ' ';
    csound->MessageS(csound, CSOUNDMSG_ERROR, "%c", ch);
  }
  csound->ErrorMsg(csound, "^");
}

static void lexerr(CSOUND *csound, const char *s, ...)
{
  IN_STACK  *curr = ST(str);
  va_list   args;

  va_start(args, s);
  csound->ErrMsgV(csound, Str("error:  "), s, args);
  va_end(args);

  while (curr != ST(inputs)) {
    if (curr->string) {
      MACRO *mm = ST(macros);
      while (mm != curr->mac) mm = mm->next;
      csound->ErrorMsg(csound, Str("called from line %d of macro %s"),
                       curr->line, mm->name);
    }
    else {
      csound->ErrorMsg(csound, Str("in line %d of file input %s"),
                       curr->line, curr->body);
    }
    curr--;
  }
  csound->LongJmp(csound, 1);
}

static void printgroups(CSOUND *csound, int grpcnt)
{                                       /*   debugging aid (onto stdout) */
  char    c, *cp = ST(group)[0];

  csound->Message(csound, "groups:\t");
  while (grpcnt--) {
    csound->Message(csound, "%s ", cp);
    while ((c = *cp++));
  }
  csound->Message(csound, "\n");
}
