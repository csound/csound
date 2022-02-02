/*
    sread.c:

    Copyright (C) 1991, 1997 Barry Vercoe, John ffitch

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

#include "csoundCore.h"                             /*   SREAD.C     */
#include <math.h>      /* for fabs() */
#include <ctype.h>
#include <inttypes.h>
#include "namedins.h"           /* IV - Oct 31 2002 */
#include "corfile.h"
#include "Engine/score_param.h"

#define MEMSIZ  16384           /* size of memory requests from system  */
#define MARGIN  4096            /* minimum remaining before new request */
#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */

//#define MACDEBUG (1)

static void print_input_backtrace(CSOUND *csound, int needLFs,
                                  void (*msgfunc)(CSOUND*, const char*, ...));
static  void    copylin(CSOUND *), copypflds(CSOUND *);
static  void    ifa(CSOUND *), setprv(CSOUND *);
static  void    carryerror(CSOUND *), pcopy(CSOUND *, int, int, SRTBLK*);
static  void    salcinit(CSOUND *);
static  void    salcblk(CSOUND *), flushlin(CSOUND *);
static  int     getop(CSOUND *), getpfld(CSOUND *, int);
        MYFLT   stof(CSOUND *, char *);
extern  void    *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);
extern int csound_prslex_init(void *);
extern void csound_prsset_extra(void *, void *);

extern int csound_prslex(CSOUND*, void*);
extern int csound_prslex_destroy(void *);
extern void cs_init_smacros(CSOUND*, PRS_PARM*, NAMES*);

#define STA(x)  (csound->sread.x)

static intptr_t expand_nxp(CSOUND *csound)
{
    char      *oldp;
    SRTBLK    *p;
    intptr_t  offs;
    size_t    nbytes;

    if (UNLIKELY((csound->sread.nxp) >=
                 ((csound->sread.memend) + MARGIN))) {
      csound->Die(csound, Str("sread:  text space overrun, increase MARGIN"));
      return 0;     /* not reached */
    }
    /* calculate the number of bytes to allocate */
    nbytes = (size_t) ((csound->sread.memend) -
                       (csound->sread.curmem));
    nbytes = nbytes + (nbytes >> 3) + (size_t) (MEMSIZ - 1);
    nbytes &= ~((size_t) (MEMSIZ - 1));
    /* extend allocated memory */
    oldp = (csound->sread.curmem);
    (csound->sread.curmem) =
      (char*) csound->ReAlloc(csound, (csound->sread.curmem),
                              nbytes + (size_t) MARGIN);
    (csound->sread.memend) =
      (char*) (csound->sread.curmem) + (int32) nbytes;
    /* did the pointer change ? */
    if ((csound->sread.curmem) == oldp)
      return (intptr_t) 0;      /* no, nothing to do */
    /* correct all pointers for the change */
    offs = (intptr_t) ((uintptr_t)(csound->sread.curmem) - (uintptr_t) oldp);
    if ((csound->sread.bp) != NULL)
      (csound->sread.bp) =
        (SRTBLK*) ((uintptr_t) (csound->sread.bp) + (intptr_t) offs);
    if ((csound->sread.prvibp) != NULL)
      (csound->sread.prvibp) =
        (SRTBLK*) ((uintptr_t) (csound->sread.prvibp) + (intptr_t) offs);
    if ((csound->sread.sp) != NULL)
      (csound->sread.sp) =
        (char*) ((uintptr_t) (csound->sread.sp) + (intptr_t) offs);
    if ((csound->sread.nxp) != NULL)
      (csound->sread.nxp) =
        (char*) ((uintptr_t) (csound->sread.nxp) + (intptr_t) offs);
    if (csound->frstbp == NULL)
      return offs;
    p = csound->frstbp;
    csound->frstbp = p = (SRTBLK*) ((uintptr_t) p + (intptr_t) offs);
    do {
      if (p->prvblk != NULL)
        p->prvblk = (SRTBLK*) ((uintptr_t) p->prvblk + (intptr_t) offs);
      if (p->nxtblk != NULL)
        p->nxtblk = (SRTBLK*) ((uintptr_t) p->nxtblk + (intptr_t) offs);
      p = p->nxtblk;
    } while (p != NULL);
    /* return pointer change in bytes */
    return offs;
}

/* sreaderr() - for non-fatal "warnings" */
static void sreaderr(CSOUND *csound, const char *s, ...)
{
    va_list   args;

    csoundMessage(csound, Str("sread: "));
    va_start(args, s);
    csoundMessageV(csound, 0, s, args);
    va_end(args);
    csoundMessage(csound, "\n");
    print_input_backtrace(csound, 1, csoundMessage);
    return;
}

/* scorerr() - for fatal errors in score parsing */
static void scorerr(CSOUND *csound, const char *s, ...)
{
    va_list   args;

    va_start(args, s);
    csound->ErrMsgV(csound, Str("score error:  "), s, args);
    va_end(args);
    print_input_backtrace(csound, 0, csoundErrorMsg);
    csound->LongJmp(csound, 1);
}

static void print_input_backtrace(CSOUND *csound, int needLFs,
                                  void (*msgfunc)(CSOUND*, const char*, ...))
{
    IN_STACK  *curr = (csound->sread.str);
    char      *m, *lf = (needLFs ? "\n" : "");
    int       lastinput = 0;
    int       lastsource = 2; /* 2=current file, 1=macro, 0=#include */

    msgfunc(csound, Str("  section %d:  at position %d%s"), csound->sectcnt,
                    (csound->sread.linepos), lf);

    do {
      if (curr == (csound->sread.inputs)) lastinput = 1;
      if (UNLIKELY(!curr->mac || !curr->mac->name)){
        csound->Warning(csound, Str("Internal error in print_input_backtrace()"));
        return;
      }
      switch(lastsource) {
      case 0: m = Str("  included from line %d of macro %s%s"); break;
      case 1: m = Str("  called from line %d of macro %s%s"); break;
        //default:
      case 2: m = Str("  in line %d of macro %s%s"); break;
      }
      msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line),
              curr->mac->name, lf);  /* #include is one line before */
      if (lastinput && csound->oparms->useCsdLineCounts && csound->csdname) {
        /* print name & line # of CSD instead of temp sco */
        msgfunc(csound, m,
                (lastsource == 0 ? csound->scoLineOffset + curr->line - 1 :
                 csound->scoLineOffset + curr->line), csound->csdname, lf);
      }
      /* else { */
      /*   msgfunc(csound, m, (lastsource == 0 ? curr->line - 1 : curr->line), */
      /*     corfile_tell(curr->cf), lf);  /\* #include is one line before *\/ */
      /* } */
    } while (!lastsource);
    curr--;
    return;
}

static MYFLT operate(CSOUND *csound, MYFLT a, MYFLT b, char c)
{
    MYFLT ans;
    extern MYFLT MOD(MYFLT,MYFLT);

    switch (c) {
    case '+': ans = a + b; break;
    case '-': ans = a - b; break;
    case '*': ans = a * b; break;
    case '/': ans = a / b; break;
    case '%': ans = MOD(a, b); break;
    case '^': ans = POWER(a, b); break;
    case '&': ans = (MYFLT) (MYFLT2LRND(a) & MYFLT2LRND(b)); break;
    case '|': ans = (MYFLT) (MYFLT2LRND(a) | MYFLT2LRND(b)); break;
    case '#': ans = (MYFLT) (MYFLT2LRND(a) ^ MYFLT2LRND(b)); break;
    default:
      csoundDie(csound, Str("Internal error op=%c"), c);
      ans = FL(0.0);    /* compiler only */
    }
    return ans;
}

static inline int isNameChar(int c, int pos)
{
    //c = (int) ((unsigned char) c);
    if (UNLIKELY(c<0)) return 0;
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

/* Functions to read/unread chracters from
 * a stack of file and macro inputs */

static inline void ungetscochar(CSOUND *csound, int c)
{
    corfile_ungetc(csound->expanded_sco);
    csound->expanded_sco->body[csound->expanded_sco->p] = (char)c;
}

static int getscochar(CSOUND *csound, int expand)
{
/* Read a score character, expanding macros if flag set */
    int     c;
    IGN(expand);
/* Read a score character, expanding macros expanded */
    c = corfile_getc(csound->expanded_sco);
    if (c == EOF) {
      if ((csound->sread.str) == &(csound->sread.inputs)[0]) {
        return EOF;
      }
    }
#ifdef MACDEBUG
    csound->DebugMsg(csound,"%s(%d): character = %c(%.2d)\n",
                     __FILE__, __LINE__, c, c);
#endif
    if (c == '\n') {
      (csound->sread.str)->line++; (csound->sread.linepos) = -1;
    }
    else (csound->sread.linepos)++;
    return c;
}

void sread_initstr(CSOUND *csound, CORFIL *sco)
{
    /* sread_alloc_globals(csound); */
    IGN(sco);
    (csound->sread.inputs) =
      (IN_STACK*) csound->Malloc(csound, 20 * sizeof(IN_STACK));
    (csound->sread.input_size) = 20;
    (csound->sread.input_cnt) = 0;
    (csound->sread.str) = (csound->sread.inputs);
    (csound->sread.str)->is_marked_repeat = 0;
    (csound->sread.str)->line = 1; (csound->sread.str)->mac = NULL;
    //init_smacros(csound, csound->smacros);
    {
      PRS_PARM  qq;
      memset(&qq, '\0', sizeof(PRS_PARM));
      csound_prslex_init(&qq.yyscanner);
      cs_init_smacros(csound, &qq, csound->smacros);
      csound_prsset_extra(&qq, qq.yyscanner);
      csound->expanded_sco = corfile_create_w(csound);
      /* printf("Input:\n%s<<<\n", */
      /*        corfile_body(csound->sread.str->cf)); */
      csound_prslex(csound, qq.yyscanner);
      csound->DebugMsg(csound, "yielding >>%s<<\n",
                       corfile_body(csound->expanded_sco));
      csound_prslex_destroy(qq.yyscanner);
      corfile_rm(csound, &csound->scorestr);
      corfile_rewind(csound->expanded_sco);
    }
}

int sread(CSOUND *csound)       /*  called from main,  reads from SCOREIN   */
{                               /*  each score statement gets a sortblock   */
    int  rtncod;                /* return code to calling program:      */
                                /*   1 = section read                   */
                                /*   0 = end of file                    */
    /* sread_alloc_globals(csound); */
    (csound->sread.bp) =
      (csound->sread.prvibp) = csound->frstbp = NULL;
    (csound->sread.nxp) = NULL;
    (csound->sread.warpin) = 0;
    (csound->sread.lincnt) = 1;
    csound->sectcnt++;
    rtncod = 0;
    salcinit(csound);           /* init the mem space for this section  */
#ifdef never
    if (csound->score_parser) {
      extern int scope(CSOUND*);
      printf("**********************************************************\n");
      printf("*******************EXPERIMENTAL CODE**********************\n");
      printf("**********************************************************\n");
      scope(csound);
      exit(0);
    }
#endif
    //printf("sread starts with >>%s<<\n", csound->expanded_sco->body);
    while (((csound->sread.op) = getop(csound)) != EOF) {
      /* read next op from scorefile */
      rtncod = 1;
      salcblk(csound);          /* build a line structure; init bp,nxp  */
    again:
      //printf("*** reading: %c (%.2x)\n",
      //       (csound->sread.op), (csound->sread.op));
      switch ((csound->sread.op)) { /*  and dispatch on opcodes  */
      case 'y':
        {
          char  *p = &((csound->sread.bp)->text[1]);
          char q;
          //char *old_nxp = (csound->sread.nxp)-2;
          //printf("text=%s<<\n", (csound->sread.bp)->text);
          /* Measurement shows isdigit and 3 cases is about 30% */
          /* faster than use of strchr (measured on Suse9.3)    */
          /*         if (strchr("+-.0123456789", *p) != NULL) { */
          while ((q=getscochar(csound,1))!='\n') *p++ = q;
          *p = '\0';
          //printf("text=%s<<\n", (csound->sread.bp)->text);
          p = &((csound->sread.bp)->text[1]);
          while (isblank(q=*p)) p++;
          if (isdigit(q) || q=='+' || q=='-' || q=='.') {
            double  tt;
            char    *tmp = p;
            tt = cs_strtod(p, &tmp);
            //printf("tt=%lf q=%c\n", tt, q);
            csound->randSeed1 = (int)tt;
            printf("seed from score %d\n", csound->randSeed1);
          }
          else {
            uint32_t tmp = (uint32_t) csound->GetRandomSeedFromTime();
            while (tmp >= (uint32_t) 0x7FFFFFFE)
              tmp -= (uint32_t) 0x7FFFFFFE;
            csound->randSeed1 = tmp+1;
            printf("seed from clock %d\n", csound->randSeed1);
          }
          //printf("cleaning up\n");
          break;
          //q = (csound->sread.op) = getop(csound);
          //printf("next op = %c(%.2x)\n", q, q);
          //goto again;
        }
      case 'i':
      case 'd':
      case 'f':
      case 'a':
      case 'q':
        ifa(csound);
        break;
      case 'w':
        (csound->sread.warpin)++;
        copypflds(csound);
        break;
      case 't':
        copypflds(csound);
        break;
      case 'B':
      case 'b': /* Set a clock base */
        {
          char *old_nxp = (csound->sread.nxp)-2;
          getpfld(csound,0);
          if (csound->sread.op == 'b')
            (csound->sread.clock_base) =
              stof(csound, (csound->sread.sp));
          else
            (csound->sread.clock_base) +=
              stof(csound, (csound->sread.sp));

          if (csound->oparms->msglevel & CS_TIMEMSG)
            csound->Message(csound,Str("Clockbase = %f\n"),
                            csound->sread.clock_base);
          flushlin(csound);
          (csound->sread.op) = getop(csound);
          (csound->sread.nxp) = old_nxp;
          *(csound->sread.nxp)++ =
            (csound->sread.op); /* Undo this line */
          (csound->sread.nxp)++;
          goto again;
        }
      case 'C':                 /* toggle carry */
        {
          char *old_nxp = (csound->sread.nxp)-2;
          getpfld(csound,0);
          (csound->sread.nocarry) =
            stof(csound, (csound->sread.sp))==0.0?1:0;
          //printf("nocarry = %d\n", (csound->sread.nocarry));
          flushlin(csound);
          (csound->sread.op) = getop(csound);
          (csound->sread.nxp) = old_nxp;
          *(csound->sread.nxp)++ =
            (csound->sread.op); /* Undo this line */
          (csound->sread.nxp)++;
          goto again;
        }
      case 's':
      case 'e':
        /* check for optional p1 before doing repeats */
        copylin(csound);
        {
          char  *p = &((csound->sread.bp)->text[1]);
          char q;
          while (isblank(*p))
            p++;
          /* Measurement shows isdigit and 3 cases is about 30% */
          /* faster than use of strchr (measured on Suse9.3)    */
          /*         if (strchr("+-.0123456789", *p) != NULL) { */
          q = *p;
          if (isdigit(q) || q=='+' || q=='-' || q=='.') {
            double  tt;
            char    *tmp = p;
            tt = cs_strtod(p, &tmp);
            if (tmp != p && (*tmp == '\0' || isspace(*tmp))) {
              (csound->sread.bp)->pcnt = 1;
              (csound->sread.bp)->p1val =
                (csound->sread.bp)->p2val =
                (csound->sread.bp)->newp2 = (MYFLT) tt;
            }
          }
          else (csound->sread.bp)->p1val =
                 (csound->sread.bp)->p2val =
                 (csound->sread.bp)->newp2 = FL(0.0);
        }
        /* If we are in a repeat of a marked section ('n' statement),
           we must pop those inputs before doing an 'r' repeat. */
        if ((csound->sread.str)->is_marked_repeat) {
          //printf("end of n; return to %d\n", (csound->sread.str)->oposit);
          corfile_set(csound->expanded_sco, (csound->sread.str)->oposit);
          (csound->sread.str)--;
          return rtncod;
        }
        /* while ((csound->sread.str)->is_marked_repeat && */
        /*        (csound->sread.input_cnt) > 0) { */
        /*   /\* close all marked repeat inputs *\/ */
        /*   //corfile_rm(&((csound->sread.str)->cf)); */
        /*   (csound->sread.str)--; (csound->sread.input_cnt)--; */
        /* } */
        /* if ((csound->sread.repeat_cnt) != 0) { */
        /*   if (do_repeat(csound)) */
        /*     return rtncod; */
        /* } */
        if ((csound->sread.op) != 'e') {
          (csound->sread.clock_base) = FL(0.0);
          (csound->sread.warp_factor) = FL(1.0);
          (csound->sread.prvp2) = -FL(1.0);
        }
        return rtncod;
      case 'm': /* Remember this place */
        {
          char  *old_nxp = (csound->sread.nxp)-2;
          char  buff[200];
          int   c;
          int   i = 0, j;
          while (isblank(c = getscochar(csound, 1)));
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          if (c != EOF && c != '\n') flushlin(csound);
          if (csound->oparms->msglevel & CS_TIMEMSG)
            csound->Message(csound,Str("m Named section >>>%s<<<\n"), buff);
            //printf("*** last_name = %d\n", (csound->sread.last_name));
          for (j=0; j<(csound->sread.last_name); j++) {
            //printf("m: %s %s(%d)\n",
            //       buff, (csound->sread.names)[j].name, j);
            if (strcmp(buff, (csound->sread.names)[j].name)==0) break;
          }
          if (j>=(csound->sread.last_name)) {
            j = ++(csound->sread.last_name);
            (csound->sread.names)[j].name =cs_strdup(csound, buff);
          }
          (csound->sread.names)[j].posit =
            corfile_tell(csound->expanded_sco);
          //printf("posit=%d\n", (csound->sread.names)[j].posit);
          (csound->sread.names)[j].line = (csound->sread.str)->line;
          //printf("line-%d\n",(csound->sread.names)[j].line);
          if (csound->oparms->msglevel & CS_TIMEMSG)
            csound->Message(csound,Str("%d: %s position %"PRIi32"\n"),
                            j, (csound->sread.names)[j].name,
                            (csound->sread.names)[j].posit);
          (csound->sread.op) = getop(csound);
          (csound->sread.nxp) = old_nxp;
          *(csound->sread.nxp)++ =
            (csound->sread.op); /* Undo this line */
          (csound->sread.nxp)++;
          goto again;           /* suggested this loses a line?? */
        }
      case 'n':
        {
          char *old_nxp = (csound->sread.nxp)-2;
          char buff[200];
          int c;
          int i = 0;
          while (isblank(c = getscochar(csound, 1)));
          while (isNameChar(c, i)) {
            buff[i++] = c;
            c = getscochar(csound, 1);
          }
          buff[i] = '\0';
          printf("n Named section %s\n", buff);
          if (c != '\n' && c != EOF) flushlin(csound);
          //printf("last_name %d\n", (csound->sread.last_name));
          for (i = 0; i<=(csound->sread.last_name); i++) {
            //printf("n: %s %s(%d)\n",
            //       buff, (csound->sread.names)[i].name, i);
            if (strcmp(buff, (csound->sread.names)[i].name)==0) break;
          }
          //printf("i=%d\n", i);
          if (UNLIKELY(i > (csound->sread.last_name)))
            sreaderr(csound, Str("Name %s not found"), buff);
          else {
            //csound->Message(csound, Str("%d: %s (%ld)\n"),
            //                i, buff, (csound->sread.names)[i].posit);
            (csound->sread.input_cnt)++;
            if (csound->sread.input_cnt>=csound->sread.input_size) {
              int old = (csound->sread.str)-(csound->sread.inputs);
              (csound->sread.input_size) += 20;
              (csound->sread.inputs) =
                csound->ReAlloc(csound, (csound->sread.inputs),
                                csound->sread.input_size * sizeof(IN_STACK));
              (csound->sread.str) =
                &(csound->sread.inputs)[old];     /* In case it moves */
            }
            (csound->sread.str)++;
            (csound->sread.str)->is_marked_repeat = 1;
            (csound->sread.str)->line = (csound->sread.names)[i].line;
            (csound->sread.str)->oposit = corfile_tell(csound->expanded_sco);
            corfile_set(csound->expanded_sco,
                        (csound->sread.names)[i].posit);
            //printf("posit was %d moved to %d\n",
            //       (csound->sread.str)->oposit,
            //       (csound->sread.names)[i].posit);
          }
              (csound->sread.op) = getop(csound);
          (csound->sread.nxp) = old_nxp;
          *(csound->sread.nxp)++ =
            (csound->sread.op); /* Undo this line */
          (csound->sread.nxp)++;
          goto again;
        }
      case 'v': /* Suggestion of Bryan Bales */
        {       /* Set local variability of time */
          char *old_nxp = (csound->sread.nxp)-2;
          getpfld(csound,0);
          (csound->sread.warp_factor) =
            stof(csound, (csound->sread.sp));
          if (csound->oparms->msglevel & CS_TIMEMSG)
            csound->Message(csound, Str("Warp_factor = %f\n"),
                            (csound->sread.warp_factor));
          flushlin(csound);
          (csound->sread.op) = getop(csound);
          (csound->sread.nxp) = old_nxp;
          *(csound->sread.nxp)++ =
            (csound->sread.op);          /* Undo this line */
          (csound->sread.nxp)++;
          goto again;
        }
      case 'x':                         /* Skip section */
        //printf("***skipping section\n");
        flushlin(csound);
        while (1) {
          switch ((csound->sread.op) = getop(csound)) {
          case 's':
          case 'r':
          case 'm':
          case 'e':
            //printf("***skip ending with %c\n", (csound->sread.op));
            salcblk(csound);            /* place op, blank into text    */
            goto again;
          case EOF:
            goto ending;
          default:
            //printf("***ignoring %c\n", (csound->sread.op));
            flushlin(csound);
          }
        }
        break;
      case -1:
        break;
      default:
        csound->Message(csound,
                        Str("sread is confused on legal opcodes %c(%.2x)\n"),
                        (csound->sread.op), (csound->sread.op));
        break;
      }
    }
 ending:
    /* if ((csound->sread.repeat_cnt) > 0) { */
    /*   (csound->sread.op) = 'e'; */
    /*   salcblk(csound); */
    /*   /\* if (do_repeat(csound)) *\/ */
    /*   /\*   return rtncod; *\/ */
    /*   *(csound->sread.nxp)++ = LF; */
    /* } */
    /* if (!rtncod) {                      /\* Ending so clear macros *\/ */
    /*   while ((csound->sread.macros) != NULL) { */
    /*     undefine_score_macro(csound, (csound->sread.macros)->name); */
    /*   } */
    /* } */
    return rtncod;
}

static void copylin(CSOUND *csound)     /* copy source line to srtblk   */
{
    int c;
    (csound->sread.nxp)--;
    if (csound->sread.nxp >= csound->sread.memend)
      /* if this memblk exhausted */
      expand_nxp(csound);
    do {
      c = getscochar(csound, 1);
      *(csound->sread.nxp)++ = c;
    } while (c != LF && c != EOF);
    if (c == EOF) *((csound->sread.nxp)-1) = '\n'; /* Avoid EOF characters */
    (csound->sread.lincnt)++;
    (csound->sread.linpos) = 0;
}

static void copypflds(CSOUND *csound)
{
    (csound->sread.bp)->pcnt = 0;
    while (getpfld(csound,1))                    /* copy each pfield,    */
      (csound->sread.bp)->pcnt++;         /* count them,          */
    *(csound->sread.nxp-1) = LF;          /* terminate with newline */
}

static void ifa(CSOUND *csound)
{
    SRTBLK *prvbp;
    int n, nocarry = 0;

    (csound->sread.bp)->pcnt = 0;
    while (getpfld(csound,0)) {   /* while there's another pfield,  */
      nocarry = 0;
      ++(csound->sread.bp)->pcnt;
      /* if (UNLIKELY(++(csound->sread.bp)->pcnt == PMAX)) { */
      /*   sreaderr(csound, Str("instr pcount exceeds PMAX")); */
      /*   csound->Message(csound, Str("      remainder of line flushed\n")); */
      /*   flushlin(csound); */
      /*   continue; */
      /* } */
      if (*(csound->sread.sp) == '^' &&
          (csound->sread.op) == 'i' &&
          (csound->sread.bp)->pcnt == 2) {
        int foundplus = 0;
        if (*((csound->sread.sp)+1)=='+') {
          (csound->sread.sp)++; foundplus = 1;
        }
        if (UNLIKELY((csound->sread.prvp2)<0)) {
          sreaderr(csound,Str("No previous event for ^"));
          (csound->sread.prvp2) =
            (csound->sread.bp)->p2val =
              (csound->sread.warp_factor) *
               stof(csound, (csound->sread.sp)+1);
        }
        else if (UNLIKELY(isspace(*((csound->sread.sp)+1)))) {
          /* stof() assumes no leading whitespace -- 070204, akozar */
          sreaderr(csound, Str("illegal space following %s, zero substituted"),
                           (foundplus ? "^+" : "^"));
          (csound->sread.prvp2) =
            (csound->sread.bp)->p2val =
               (csound->sread.prvp2);
        }
        else (csound->sread.prvp2) =
               (csound->sread.bp)->p2val =
                 (csound->sread.prvp2) + csound->sread.warp_factor *
               stof(csound, (csound->sread.sp) + 1);
      }
      else if ((csound->sread.nxp)-(csound->sread.sp) == 2 &&
               (*(csound->sread.sp) == '.' ||
                *(csound->sread.sp) == '+')) {
        if ((csound->sread.op) == 'i'
            && (*(csound->sread.sp) == '.' ||
                (csound->sread.bp)->pcnt == 2)
            && (((csound->sread.bp)->pcnt >= 2
                 && (prvbp = (csound->sread.prvibp)) != NULL
                 && (csound->sread.bp)->pcnt <= prvbp->pcnt)
                || ((csound->sread.bp)->pcnt == 1 &&
                    (prvbp = (csound->sread.bp)->prvblk) != NULL
                    && prvbp->text[0] == 'i'))) {
          if (*(csound->sread.sp) == '.') {
            (csound->sread.nxp) = (csound->sread.sp);
            pcopy(csound, (int) (csound->sread.bp)->pcnt, 1, prvbp);
            if ((csound->sread.bp)->pcnt >= 2)
              (csound->sread.prvp2) = (csound->sread.bp)->p2val;
          }
          else /* need the fabs() in case of neg p3 */
            (csound->sread.prvp2) = (csound->sread.bp)->p2val =
                        prvbp->p2val + FABS(prvbp->p3val);
        }
        else carryerror(csound);
      }
      else if (*(csound->sread.sp) == '!') {
        int getmore = 0;
        if (UNLIKELY((csound->sread.op) != 'i')) {
          *((csound->sread.nxp)-1) = '\0';
          getmore = 1;
          sreaderr(csound, Str("ignoring '%s' in '%c' event"),
                   (csound->sread.sp), (csound->sread.op));
        }
        else if (UNLIKELY((csound->sread.bp)->pcnt < 4)) {
          sreaderr(csound, Str("! invalid in p1, p2, or p3"));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else if (UNLIKELY(csound->sread.nxp-csound->sread.sp != 2)) {
          sreaderr(csound, Str("illegal character after !: '%c'"),
                   *((csound->sread.sp)+1));
          csound->Message(csound, Str("      remainder of line flushed\n"));
          flushlin(csound);
        }
        else {
          nocarry = 1;         /* only set when no syntax errors */
          flushlin(csound);
        }
        /* but always delete the pfield beginning with '!' */
        (csound->sread.nxp) = (csound->sread.sp);
        (csound->sread.bp)->pcnt--;
        if (getmore) continue; /* not the best, but not easy to delete event */
                               /* since ifa() doesn't return anything */
        else break;
      }
      else switch ((csound->sread.bp)->pcnt) { /*  watch for p1,p2,p3, */
        case 1:                           /*   & MYFLT, setinsno..*/
          if (((csound->sread.op) == 'i' ||
               (csound->sread.op) == 'd' ||
               (csound->sread.op) == 'q') &&
              *(csound->sread.sp) == '"') {
            /* csound->DebugMsg(csound,"***Entering second dubious code scnt=%d\n",
               csound->scnt0); */
            (csound->sread.bp)->p1val = SSTRCOD;      /* allow string name */
          }
          else {
            (csound->sread.bp)->p1val =
              stof(csound, (csound->sread.sp));
          }
          if ((csound->sread.op) == 'i' || (csound->sread.op) == 'd')
            setprv(csound);
          else (csound->sread.prvibp) = NULL;
          break;
        case 2: (csound->sread.prvp2) = (csound->sread.bp)->p2val =
            (csound->sread.warp_factor)*
            stof(csound, csound->sread.sp) + csound->sread.clock_base;
          break;
        case 3: if ((csound->sread.op) == 'i')
            (csound->sread.bp)->p3val =
              (csound->sread.warp_factor) *
              stof(csound, (csound->sread.sp));
          else (csound->sread.bp)->p3val =
                 stof(csound, (csound->sread.sp));
          break;
        default:break;
      }
      switch ((csound->sread.bp)->pcnt) {           /* newp2, newp3:   */
      case 2: if ((csound->sread.warpin)) {         /* for warpin,     */
          getpfld(csound,0);                   /*   newp2 follows */
          (csound->sread.bp)->newp2 =
            (csound->sread.warp_factor) *
            stof(csound, (csound->sread.sp)) +
            (csound->sread.clock_base);
          (csound->sread.nxp) = (csound->sread.sp); /* (skip text)  */
        }
        else (csound->sread.bp)->newp2 =
               (csound->sread.bp)->p2val;   /* else use p2val  */
        break;
      case 3: if ((csound->sread.warpin) &&
                  ((csound->sread.op) == 'i' ||
                   (csound->sread.op) == 'f')) {
          getpfld(csound,0);                    /* same for newp3  */
          (csound->sread.bp)->newp3 =
            (csound->sread.warp_factor) *
            stof(csound, (csound->sread.sp));
          (csound->sread.nxp) = (csound->sread.sp);
        }
        else (csound->sread.bp)->newp3 = (csound->sread.bp)->p3val;
        break;
      }
    }
    if ((csound->sread.nocarry) &&
        ((csound->sread.bp)->pcnt<3) &&
        (csound->sread.op) == 'i' &&
        ((prvbp = (csound->sread.prvibp)) != NULL ||
         (!(csound->sread.bp)->pcnt &&
          (prvbp = (csound->sread.bp)->prvblk) != NULL &&
          prvbp->text[0] == 'i'))){ /* carry p1-p3 */
      int pcnt = (csound->sread.bp)->pcnt;
      n = 3-pcnt;
      pcopy(csound, pcnt + 1, n, prvbp);
      (csound->sread.bp)->pcnt = 3;
    }
    if ((csound->sread.op) == 'i' && !nocarry && /* then carry any rem pflds */
        !(csound->sread.nocarry) &&
        ((prvbp = (csound->sread.prvibp)) != NULL ||
         (!(csound->sread.bp)->pcnt &&
          (prvbp = (csound->sread.bp)->prvblk) != NULL &&
          prvbp->text[0] == 'i')) &&
        (n = prvbp->pcnt - (csound->sread.bp)->pcnt) > 0) {
      //printf("carrying p-fields\n");
      pcopy(csound, (int) (csound->sread.bp)->pcnt + 1, n, prvbp);
      (csound->sread.bp)->pcnt += n;
    }
    *((csound->sread.nxp)-1) = LF;   /* terminate this stmnt with newline */
}

static void setprv(CSOUND *csound)      /*  set insno = (int) p1val         */
{                                       /*  prvibp = prv note, same insno   */
    SRTBLK *p = (csound->sread.bp);
    int16 n;

    if (csound->ISSTRCOD((csound->sread.bp)->p1val) &&
        *(csound->sread.sp) == '"') {
      /* IV - Oct 31 2002 */
      int sign = 0;
      char name[MAXNAME], *c, *s = (csound->sread.sp);
      /* unquote instrument name */
      c = name; while (*++s != '"') *c++ = *s; *c = '\0';
      if (*name=='-') {
        sign = 1;
        printf("negative name %s\n", name); }
      /* find corresponding insno */
      if (UNLIKELY(!(n = (int16) named_instr_find(csound, name+sign)))) {
        csound->Message(csound, Str("WARNING: instr %s not found, "
                                    "assuming insno = -1\n"), name);
        n = -1;
      }
      if (sign) n = -n;
    }
    else n = (int16) (csound->sread.bp)->p1val;         /* set current insno */
    (csound->sread.bp)->insno = n;

    while ((p = p->prvblk) != NULL)
      if (p->insno == n) {
        (csound->sread.prvibp) = p;                     /* find prev same */
        return;
      }
    (csound->sread.prvibp) = NULL;                      /*  if there is one */
}

static void carryerror(CSOUND *csound)      /* print offending text line  */
{                                           /*      (partial)             */
    char *p;

    csound->Message(csound, Str("sread: illegal use of carry, "
                                "  0 substituted\n"));
    *((csound->sread.nxp) - 3) = SP;
    p = (csound->sread.bp)->text;
    while (p <= (csound->sread.nxp) - 2)
      csound->Message(csound, "%c", *p++);
    csound->Message(csound, "<=\n");
    print_input_backtrace(csound, 1, csoundMessage);
    *((csound->sread.nxp) - 2) = '0';
}

static void pcopy(CSOUND *csound, int pfno, int ncopy, SRTBLK *prvbp)
                                /* cpy pfields from prev note of this instr */
                                /*     begin at pfno, copy 'ncopy' fields   */
                                /*     uses *nxp++;    sp untouched         */
{
    char *p, *pp, c;
    int  n;

    pp = prvbp->text;                       /* in text of prev note,    */
    n = pfno;
    while (n--)
      while (*pp++ != SP)                   /*    locate starting pfld  */
        ;
    n = ncopy;
    p = (csound->sread.nxp);
    while (n--) {                           /*      and copy n pflds    */
      if (*pp != '"')
        while ((*p++ = c = *pp++) != SP && c != LF)
          ;
      else {
        *p++ = *pp++;
        while ((*p++ = *pp++) != '"')
          ;
        *p++ = *pp++;
      }
      switch (pfno) {
      case 1: (csound->sread.bp)->p1val = prvbp->p1val;       /*  with p1-p3 vals */
        setprv(csound);
        break;
      case 2:
        if (*(p-2) == '+')              /* (interpr . of +) */
          (csound->sread.prvp2) =
            (csound->sread.bp)->p2val = prvbp->p2val + FABS(prvbp->p3val);
        else (csound->sread.prvp2) = (csound->sread.bp)->p2val = prvbp->p2val;
        (csound->sread.bp)->newp2 = (csound->sread.bp)->p2val;
        break;
      case 3: (csound->sread.bp)->newp3 = (csound->sread.bp)->p3val = prvbp->p3val;
        break;
      default:
        break;
      }
      (csound->sread.bp)->lineno = prvbp->lineno;
      pfno++;
    }
    (csound->sread.nxp) = p;            /*          adjust globl nxp pntr */
}

static void salcinit(CSOUND *csound)
{                             /* init the sorter mem space for a new section */
    if (csound->sread.curmem == NULL) { /*  alloc 1st memblk if nec;
                                            init *nxp to this */
      (csound->sread.curmem) =
        (char*) csound->Calloc(csound, (size_t) (MEMSIZ + MARGIN));
      (csound->sread.memend) = (char*) (csound->sread.curmem) + MEMSIZ;
    }
    (csound->sread.nxp) = (char*) (csound->sread.curmem);
}

static void salcblk(CSOUND *csound)
{                               /* alloc a srtblk from current mem space:   */
    SRTBLK  *prvbp;             /*   align following *nxp, set new bp, nxp  */
                                /*   set srtblk lnks, put op+blank in text  */
    if (csound->sread.nxp >= csound->sread.memend) /* if this memblk exhausted */
      expand_nxp(csound);
    /* now allocate a srtblk from this space: */
    prvbp = (csound->sread.bp);
    (csound->sread.bp) =
      (SRTBLK*) (((uintptr_t) csound->sread.nxp + (uintptr_t)7) & ~((uintptr_t)7));
    if (csound->frstbp == NULL)
      csound->frstbp = (csound->sread.bp);
    if (prvbp != NULL)
      prvbp->nxtblk = (csound->sread.bp); /* link with prev srtblk        */
    (csound->sread.bp)->nxtblk = NULL;
    (csound->sread.bp)->prvblk = prvbp;
    (csound->sread.bp)->insno = 0;
    (csound->sread.bp)->pcnt = 0;
    (csound->sread.bp)->lineno = (csound->sread.lincnt);
    (csound->sread.nxp) = &((csound->sread.bp)->text[0]);
    *(csound->sread.nxp)++ = (csound->sread.op); /* place op, blank into text    */
    *(csound->sread.nxp)++ = SP;
    *(csound->sread.nxp) = '\0';
}

void sfree(CSOUND *csound)       /* free all sorter allocated space */
{                                /*    called at completion of sort */
    /* sread_alloc_globals(csound); */
    if ((csound->sread.curmem) != NULL) {
      csound->Free(csound, (csound->sread.curmem));
      (csound->sread.curmem) = NULL;
    }
    while ((csound->sread.str) != &(csound->sread.inputs)[0]) {
      //corfile_rm(&((csound->sread.str)->cf));
      (csound->sread.str)--;
    }
    corfile_rm(csound, &(csound->scorestr));
}

static void flushlin(CSOUND *csound)
{                                   /* flush input to end-of-line; inc lincnt */
    int c;
    while ((c = getscochar(csound, 0)) != LF && c != EOF)
      ;
    (csound->sread.linpos) = 0;
    (csound->sread.lincnt)++;
}

/* unused at the moment
static inline int check_preproc_name(CSOUND *csound, const char *name)
{
    int   i;
    char  c;
    for (i = 1; name[i] != '\0'; i++) {
      c = (char) getscochar(csound, 1);
      if (c != name[i])
        return 0;
    }
    return 1;
}
*/

static int sget1(CSOUND *csound)    /* get first non-white, non-comment char */
{
    int c;

    //srch:
    while (isblank(c = getscochar(csound, 1)) || c == LF)
      if (c == LF) {
        (csound->sread.lincnt)++;
        (csound->sread.linpos) = 0;
      }
    /* Can never be ; from lexer, could do c in lexer *\/ */
    /* if (c == ';' || c == 'c') { */
    /*   flushlin(csound); */
    /*   goto srch; */
    /* } */
    //printf("***Next non white: %c(%.2x)\n", c, c);
    return c;
}

static int getop(CSOUND *csound)        /* get next legal opcode */
{
    int c;

 nextc:
    c = sget1(csound);  /* get first active char */

    switch (c) {        /*   and check legality  */
    case 'a':           /* Advance time */
    case 'b':           /* Reset base clock */
    case 'B':           /* Reset base clock accumulative*/
    case 'd':           /* De-note */
    case 'C':           /* toggle carry flag */
    case 'e':           /* End of all */
    case 'f':           /* f-table */
    case 'i':           /* Instrument */
    case 'm':           /* Mark this point */
    case 'n':           /* Duplicate from named position */
    case 'q':           /* quiet instrument ie mute */
    case 'r':           /* Repeated section */
    case 's':           /* Section */
    case 't':           /* time warp */
    case 'v':           /* Local warping */
    case 'w':
    case 'x':
    case 'y':           /* Set random seed */
    case '{':           /* Section brackets */
    case '}':
    case EOF:
      break;            /* if ok, go with it    */
    default:            /*   else complain      */
      sreaderr(csound, Str("illegal opcode %c"), c);
      csound->Message(csound,Str("      remainder of line flushed\n"));
      flushlin(csound);
      goto nextc;
    }
    (csound->sread.linpos)++;
    return(c);
}

static MYFLT read_expression(CSOUND *csound)
{
      char  stack[30];
      MYFLT vv[30];
      char  *op = stack - 1;
      MYFLT *pv = vv - 1;
      char  buffer[100];
      int   i, c;
      int   type = 0;  /* 1 -> expecting binary operator,')', or ']'; else 0 */
      *++op = '[';
      c = getscochar(csound, 1);
      do {
        //printf("read_expression: c=%c\n", c);
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '.':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of number in [] "
                                "expression"));
          }
 parseNumber:
          i = 0;
          do {
            buffer[i++] = c;
            c = getscochar(csound, 1);
          } while (isdigit(c) || c == '.');
          if (c == 'e' || c == 'E') {
            buffer[i++] = c;
            c = getscochar(csound, 1);
            if (c == '+' || c == '-') {
              buffer[i++] = c;
              c = getscochar(csound, 1);
            }
            while (isdigit(c)) {
              buffer[i++] = c;
              c = getscochar(csound, 1);
            }
          }
          buffer[i] = '\0';
          //printf("****>>%s<<\n", buffer);
          *++pv = stof(csound, buffer);
          type = 1;
          break;
        case '~':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of operator ~ in [] "
                                "expression"));
          }
          *++pv = (MYFLT) (csound->Rand31(&(csound->randSeed1)) - 1)
                  / FL(2147483645);
          type = 1;
          c = getscochar(csound, 1);
          break;
        case '@':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of operator @ or @@ in"
                                " [] expression"));
          }
          {
            int n = 0;
            int k = 0;          /* 0 or 1 depending on guard bit */
            c = getscochar(csound, 1);
            if (c=='@') { k = 1; c = getscochar(csound, 1);}
            while (isdigit(c)) {
              n = 10*n + c - '0';
              c = getscochar(csound, 1);
            }
            i = 1;
            while (i<=n-k && i< 0x4000000) i <<= 1;
            *++pv = (MYFLT)(i+k);
            type = 1;
          }
          break;
        case '+': case '-':
          if (!type)
            goto parseNumber;
          if (*op != '[' && *op != '(') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '*':
        case '/':
        case '%':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("illegal placement of operator %c in [] "
                                "expression"), c);
          }
          if (*op == '*' || *op == '/' || *op == '%') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '&':
        case '|':
        case '#':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("illegal placement of operator %c in [] "
                                "expression"), c);
          }
          if (*op == '|' || *op == '&' || *op == '#') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '(':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of '(' in [] expression"));
          }
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case ')':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("missing operand before ')' in [] expression"));
          }
          while (*op != '(') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 1;
          op--; c = getscochar(csound, 1); break;
        case '^':
          type = 0;
          *++op = c; c = getscochar(csound, 1); break;
        case '[':
          if (UNLIKELY(type)) {
            scorerr(csound, Str("illegal placement of '[' in [] expression"));
          }
          type = 1;
          {
            //int i;
            MYFLT x;
            //for (i=0;i<=pv-vv;i++) printf(" %lf ", vv[i]);
            //printf("| %d\n", pv-vv);
            x = read_expression(csound);
            *++pv = x;
            //printf("recursion gives %lf (%lf)\n", x,*(pv-1));
            //for (i=0;i<pv-vv;i++) printf(" %lf ", vv[i]); printf("| %d\n", pv-vv);
            c = getscochar(csound, 1); break;
          }
        case ']':
          if (UNLIKELY(!type)) {
            scorerr(csound, Str("missing operand before closing bracket in []"));
          }
          while (*op != '[') {
            MYFLT v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          //printf("done ]*** *op=%c v=%lg (%c)\n", *op, *pv, c);
          //getscochar(csound, 1);
          /* c = '$'; */ return *pv;
        case '$':
          break;
        case ' ':               /* Ignore spaces */
          c = getscochar(csound, 1);
          continue;
        default:
          scorerr(csound, Str("illegal character %c(%.2x) in [] expression"),
                  c, c);
        }
      } while (c != '$');
      return *pv;
}


static int getpfld(CSOUND *csound, int type) /* get pfield val from SCOREIN file */
{                                            /*      set sp, nxp                 */
    int  c;
    char *p;

    if ((c = sget1(csound)) == EOF)     /* get 1st non-white,non-comment c  */
      return(0);
    if (c=='[') {
      MYFLT xx = read_expression(csound);
      //printf("****xx=%a\n", xx);
      //printf("nxp = %p\n", (csound->sread.nxp));
      snprintf((csound->sread.sp) = (csound->sread.nxp), 28, "%a$", xx);
      p = strchr((csound->sread.sp),'$');
      goto blank;
    }

                    /* if non-numeric, and non-carry, and non-special-char: */
    /*    if (strchr("0123456789.+-^np<>()\"~!", c) == NULL) { */
    if (!isdigit(c) && (type || (c!='.' && c!='+' && c!='-' && c!='^' && c!='n'
                                 && c!='p' && c!='<' && c!='>' && c!='(' && c!=')'
                                 && c!='"' && c!='~' && c!='!' && c!='z'))) {
      ungetscochar(csound, c);                /* then no more pfields    */
      if (UNLIKELY((csound->sread.linpos))) {
        sreaderr(csound, Str("unexpected char %c"), c);
        csound->Message(csound, Str("      remainder of line flushed\n"));
        flushlin(csound);
      }
      return(0);                              /*    so return            */
    }
    p = (csound->sread.sp) = (csound->sread.nxp); /* else start copying to text */
    *p++ = c;
    (csound->sread.linpos)++;
    if (c == '"') {                           /* if have quoted string,  */
      /* IV - Oct 31 2002: allow string instr name for i and q events */
      if (UNLIKELY((csound->sread.bp)->pcnt < 3 &&
          !(((csound->sread.op) == 'i' ||
             (csound->sread.op) == 'd' || (csound->sread.op) == 'q') &&
            !(csound->sread.bp)->pcnt))) {
        sreaderr(csound, Str("illegally placed string"));
        csound->Message(csound, Str("      remainder of line flushed\n"));
        flushlin(csound);
        return(0);
      }
      while ((c = getscochar(csound, 1)) != '"') {
        if (UNLIKELY(c == LF || c == EOF)) {
          sreaderr(csound, Str("unmatched quote"));
          return(0);
        }
        *p++ = c;                       /*   copy to matched quote */
        if (c=='\\') {
          *p++ = getscochar(csound, 1);
          //          printf("escaped %c\n", *(p-1));
        }
        /* **** CHECK **** */
        if (p >= (csound->sread.memend))
          p = (char*) ((uintptr_t) p + expand_nxp(csound));
        /* **** END CHECK **** */
      }
      *p++ = c;
      goto blank;
    }
    while (1) {
      c = getscochar(csound, 1);
      /* else while legal chars, continue to bld string */
      /*      if (strchr("0123456789.+-eEnp<>()~", c) != NULL) { */
      if (isdigit(c) || c=='.' || c=='+' || c=='-' || c=='e' ||
          c=='E' || c=='n' || c=='p' || c=='<' || c=='>' || c=='(' ||
          c==')' || c=='~' || c=='z') {
        *p++ = c;
        /* **** CHECK **** */
        if (p >= (csound->sread.memend))
          p = (char*) ((uintptr_t) p + expand_nxp(csound));
        /* **** END CHECK **** */
      }
      else {                                /* any illegal is delimiter */
        ungetscochar(csound, c);
        break;
      }
    }
 blank:
    *p++ = SP;
    (csound->sread.nxp) = p;                            /*  add blank      */
    return(1);                              /*  and report ok  */
}

MYFLT stof(CSOUND *csound, char s[])            /* convert string to MYFLT  */
                                    /* (assumes no white space at beginning */
{                                   /*      but a blank or nl at end)       */
    char    *p;
    MYFLT   x = (MYFLT) cs_strtod(s, &p);
    if (*p=='z') return FL(800000000000.0); /* 25367 years */
    if (UNLIKELY(s == p || !(*p == '\0' || isspace(*p)))) {
      csound->Message(csound, Str("sread: illegal number format:  "));
      p = s;
      while (!(*p == '\0' || isspace(*p))) {
        csound->Message(csound, "%c", *p);
        *p++ = '0';
      }
      csound->Message(csound,Str("   zero substituted.\n"));
      print_input_backtrace(csound, 1, csoundMessage);
      return FL(0.0);
    }
    return x;
}
