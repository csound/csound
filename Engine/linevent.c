/*
    linevent.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, matt ingalls

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

#include "cs.h"             /*                              LINEVENT.C      */
#include <stdlib.h>
#include <ctype.h>
#if defined(mills_macintosh) || defined(SYMANTEC)
#include <console.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "linevent.h"

void RTclose(void);
extern int close(int);
#if !defined(mac_classic) && !defined(SYMANTEC) && !defined(LINUX) && !defined(__MACH__)
extern int read(int, void*, unsigned);
#endif

#ifdef PIPES
# if defined(SGI) || defined(LINUX) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# elif defined(__BEOS__) || defined(__MACH__)
#  include <stdio.h>
#  define _popen popen
#  define _pclose pclose
# else
   FILE *_popen(const char *, const char *);
# endif
#endif

#define LBUFSIZ   32768
#define LF        '\n'

static char *Linebuf = NULL, *Linep = NULL, *Linebufend = NULL;
#if defined(mills_macintosh) || defined(SYMANTEC)
static FILE *Linecons;
#endif

typedef struct levtblk {
    struct levtblk *nxtblk;
    struct levtblk *nxtact;
    long      inuse, oncounter;
    EVTBLK    evtblk;
} LEVTBLK;

static LEVTBLK  *Firstblk = NULL;
static LEVTBLK  *Firstact = NULL;

extern OPARMS O;
extern ENVIRON cenviron;
int stdmode;

void RTLineset(void)   /* set up Linebuf & ready the input files */
{                      /*     callable once from musmon.c        */
    Firstblk = (LEVTBLK *) mcalloc(&cenviron, (long)sizeof(LEVTBLK));
    Firstact = NULL;
    Linebuf = mcalloc(&cenviron, (long)LBUFSIZ);
    Linebufend = Linebuf + LBUFSIZ;
    Linep = Linebuf;
    if (strcmp(O.Linename,"stdin") == 0) {
#ifdef SYMANTEC
      console_options.top += 10;
      console_options.left += 10;
      console_options.title = "\pRT Line_events";
      console_options.nrows = 10;
      console_options.ncols = 50;
      Linecons = fopenc();
      cshow(Linecons);
#elif defined(mills_macintosh)
      Linecons = stdin;
      setvbuf(stdin, NULL, _IONBF, 0);
#else
#if defined(DOSGCC) || defined(__WATCOMC__) || defined(WIN32) || defined(mac_classic)
      setvbuf(stdin, NULL, _IONBF, 0);
      /*      WARNING("-L stdin:  system has no fcntl function to get stdin"); */
#else
      if (fcntl(Linefd, F_SETFL,
                (stdmode =fcntl(Linefd, F_GETFL, 0)) | O_NDELAY) < 0)
        die(Str("-L stdin fcntl failed"));
#endif
#endif
    }
#ifdef PIPES
    else if (O.Linename[0]=='|') {
      Linepipe = _popen(&(O.Linename[1]), "r");
      if (Linepipe == NULL) {
        FILE *xxx = Linepipe;
        Linefd = fileno(xxx);
      }
      else dies(Str("Cannot open %s"), O.Linename);
    }
#endif
#if defined(mills_macintosh) || defined(SYMANTEC)
#define MODE
#else
#define MODE ,0
#endif
    else if ((Linefd = open(O.Linename, O_RDONLY | O_NDELAY  MODE)) < 0)
      dies(Str("Cannot open %s"), O.Linename);
    printf(Str("stdmode = %.8x Linefd = %d\n"), stdmode, Linefd);
    atexit(RTclose);
}

void RTclose(void)
{
    printf(Str("stdmode = %.8x Linefd = %d\n"), stdmode, Linefd);
#if defined(mills_macintosh) || defined(SYMANTEC)
    if (Linecons != NULL) fclose(Linecons);
#else
                                /* JPff patch hoping to keep window */
    if (Linefd)
      close(Linefd);
    else {
#if !defined(DOSGCC) && !defined(__WATCOMC__) && !defined(WIN32) && !defined(mac_classic)
        fcntl(Linefd, F_SETFL, stdmode);
#endif
    }
#endif
    mfree(&cenviron, Linebuf);
}

static int containsLF(char *cp, char *endp)/* does string segment contain LF? */
{
    do {
      if (*cp++ == LF)  return(1);
    } while (cp < endp);
    return(0);
}

static LEVTBLK *getblk(void)/* get blk from the LEVTBLK pool, or alloc a new one */
{
    LEVTBLK *curp = Firstblk, *nxtp;

    while (curp->inuse) {
      if ((nxtp = curp->nxtblk) == NULL) {
        nxtp = (LEVTBLK *) mcalloc(&cenviron, (long)sizeof(LEVTBLK));
        curp->nxtblk = nxtp;
      }
      curp = nxtp;
    }
    return(curp);
}

static void Linsert(LEVTBLK *curp) /* insert blk into the time-ordered linevent queue */
{
    LEVTBLK *nxtp = Firstact, *prvp = NULL;

    while (nxtp != NULL && nxtp->inuse && nxtp->oncounter <= curp->oncounter) {
      prvp = nxtp;
      nxtp = nxtp->nxtact;
    }
    if (prvp == NULL)
        Firstact = curp;
    else prvp->nxtact = curp;
    curp->nxtact = nxtp;
}

/* insert text from an external source, to be interpreted as if
    coming in from stdin/Linefd for -L*/
void writeLine(const char *text, long size)
{
    if (Linebuf) {
      if ((Linep + size) < Linebufend) {
        memcpy(Linep, text, size);
        Linep += size;
      }
      else {
        if (O.msglevel & WARNMSG)
          printf(Str("WARNING: LineBuffer Overflow - Input Data has been Lost\n"));
      }
    }
    else {
      if (O.msglevel & WARNMSG)
        printf(Str(
                  "WARNING: Input ignored, RT Line Events (-L) has not been initialised\n"));
    }
}

int sensLine(void)
    /* accumlate RT Linein buffer, & place completed events in EVTBLK */
{   /* does more syntax checking than rdscor, since not preprocessed  */
    int c;
    char *cp;
    LEVTBLK  *Curblk;
    EVTBLK   *e;
    int      n, pcnt;
    MYFLT    *fp, *prvfp;
    char     *Linend;
    static EVTBLK *prve = NULL;
    static char prvop = ' ';
    static int prvpcnt = 0;
    static MYFLT prvp1 = FL(0.0);
    static MYFLT prvp2 = FL(0.0);

    if (Linefd >= 0 && (
#if defined(mills_macintosh) || defined(SYMANTEC)
                        ((n = fread((void *)Linep, (size_t)1,
                                    (size_t)(Linebufend-Linep), Linecons)) > 0) ||
#else
                        ((n = read(Linefd, Linep, Linebufend-Linep)) > 0) ||
#endif
                        Linep > Linebuf))
      {
        /*      printf("sensLine %d CHARS\n",n);  */
        Linend = Linep + (n>0 ? n : 0);
        /*        if ((c = *(Linend - 1)) == LF || containsLF(Linep,Linend)) { */
        if (containsLF(Linebuf,Linend)) {
          cp = Linebuf;
          Curblk = getblk();          /* get a blk from the levtblk pool */
          e = &Curblk->evtblk;
          while ((c = *cp++) == ' ' || c == '\t')/* skip initial white space */
            ;
          if (c == LF) goto Timchk;     /* if null line, bugout     */
          switch (c) {                  /* look for legal opcode    */
          case 'e':                     /* Quit realtime */
          case 'i':
          case 'f':
            e->opcod = c;
            break;
          default:
            err_printf(Str("unknown opcode %c\n"));
            goto Lerr;
          }                                  /* for params that follow:  */
          for (fp = &e->p[1], pcnt = 0; c != LF && pcnt<PMAX; pcnt++) {
            /*          long val = 0, scale = 1; */
            char *newcp;
            while ((c = *cp++) == ' ' || c == '\t') /* skip white space */
              ;
            if (c == LF) break;
            if (c == '"') {                /* if find character string  */
              char *sstrp;      /* IV - Oct 31 2002: allow string instrname */
              if (pcnt != 4 && (pcnt || e->opcod != 'i')) {
                err_printf(Str("misplaced string\n")); /* (must be p5) */
                goto Lerr;
              }
              if ((sstrp = e->strarg) == NULL)
                e->strarg = sstrp = mcalloc(&cenviron, (long)SSTRSIZ);
              while ((c = *cp++) != '"') {
                if (c == LF) {
                  err_printf(Str("unmatched quotes\n"));
                  cp--;
                  goto Lerr;
                }
                *sstrp++ = c;           /*   sav in private strbuf */
              }
              *sstrp = '\0';
              *fp++ = SSTRCOD;            /*   & store coded float   */
              continue;
            }
            if (!(isdigit(c) || c=='+' || c=='-' || c=='.'))
              goto Lerr;
            if (c == '.'                        /*  if lone dot,       */
                && ((n = *cp)==' ' || n=='\t' || n==LF)) {
              if (e->opcod != 'i' || prvop != 'i' || pcnt >= prvpcnt) {
                err_printf(Str("dot carry has no reference\n"));
                goto Lerr;
              }
              if (e != prve) {                /*        pfld carry   */
                if (pcnt == 1)
                  *fp = prvp2;
                else *fp = prve->p[pcnt+1];
              }
              fp++;
              continue;
            }
            *fp++ = (MYFLT)strtod(cp-1, &newcp);
            cp = newcp;
          }
          if (e->opcod == 'i' && prvop == 'i') /* do carries for instr data */
            if (!pcnt || (pcnt < prvpcnt && e->p[1] == prvp1)) {
              int pcntsav = pcnt;
              if (e != prve)
                for (prvfp = &prve->p[pcnt+1]; pcnt < prvpcnt; pcnt++)
                  *fp++ = *prvfp++;
              else pcnt =  prvpcnt;
              if (pcntsav < 2)
                e->p[2] = prvp2;
            }
          if (pcnt < 3 && e->opcod !='e') {     /* check sufficient pfields */
            err_printf(Str("too few pfields\n"));
            goto Lerr;
          }
          e->pcnt = pcnt;                      /*   &  record pfld count    */
          prve = e;
          prvop = e->opcod;                    /* preserv the carry sensors */
          prvpcnt = pcnt;
          prvp1 = e->p[1];
          prvp2 = e->p[2];
          if (pcnt == PMAX && c != LF) {
            err_printf(Str("too many pfields\n"));
            while (*cp++ != LF)              /* flush any excess data     */
              ;
          }
          Linep = Linebuf;
          while (cp < Linend)                  /* copy remaining data to    */
            *Linep++ = *cp++;                /*     beginning of Linebuf  */
          if (e->p[2] < 0.) {
            err_printf(Str("-L with negative p2 illegal\n"));
            goto Lerr;
          }
          e->p2orig = e->p[2];
          e->p3orig = e->p[3];
          Curblk->inuse = 1;
          Curblk->oncounter = global_kcounter + (long)(e->p[2] * global_ekr);
          Linsert(Curblk);
        }
        else Linep += n;           /* else just accum the chars */
      }

 Timchk:
    if (Firstact != NULL &&
        Firstact->oncounter <= global_kcounter) {    /* if an event is due now,*/
      Linevtblk = &Firstact->evtblk;
      Firstact->inuse = 0;                    /*   mark its space available    */
      Firstact = Firstact->nxtact;
      return(1);                              /*   & report Line-type RTevent  */
    }
    else return(0);                           /* else nothing due yet          */

 Lerr:
    n = cp - Linebuf - 1;                     /* error position */
    while (*cp++ != LF);                      /* go on to LF    */
    *(cp-1) = '\0';                           /*  & insert NULL */
    err_printf(Str("illegal RT scoreline:\n%s\n"), Linebuf);
    while (n--)
      err_printf(" ");
    err_printf("^\n");                        /* mark the error */
    Linep = Linebuf;
    while (cp < Linend)
      *Linep++ = *cp++;                       /* mov rem data forward */
    return(0);
}

/* send a lineevent from the orchestra */
/* code derived from sensLine() -matt 2001/12/07 */

int eventOpcode(ENVIRON *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i;

    if (*p->args[0] != SSTRCOD || p->STRARG == NULL ||
        (*p->STRARG != 'i' && *p->STRARG != 'q' && *p->STRARG != 'f' &&
         *p->STRARG != 'e'))
      return
        perferror(Str("event param 1 must be \"i\", \"q\", \"f\", or \"e\""));
    evt.strarg = NULL;
    evt.opcod = *p->STRARG;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0 && *p->args[1] == SSTRCOD) {
      if (evt.opcod != 'i' && evt.opcod != 'q')
        return perferror(Str("event: string name is allowed only for "
                             "\"i\" and \"q\" events"));
      evt.strarg = p->STRARG2;
    }
    for (i = 1; i <= evt.pcnt; i++)
      evt.p[i] = *p->args[i];
    return (insert_score_event(csound, &evt,
                               csound->sensEvents_state.curTime, 0) == 0 ?
            OK : NOTOK);
}

/* i-time version of event opcode */

int eventOpcodeI(ENVIRON *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i;

    if (*p->args[0] != SSTRCOD || p->STRARG == NULL ||
        (*p->STRARG != 'i' && *p->STRARG != 'q' && *p->STRARG != 'f' &&
         *p->STRARG != 'e'))
      return
        initerror(Str("event param 1 must be \"i\", \"q\", \"f\", or \"e\""));
    evt.strarg = NULL;
    evt.opcod = *p->STRARG;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0 && *p->args[1] == SSTRCOD) {
      if (evt.opcod != 'i' && evt.opcod != 'q')
        return initerror(Str("event: string name is allowed only for "
                             "\"i\" and \"q\" events"));
      evt.strarg = p->STRARG2;
    }
    for (i = 1; i <= evt.pcnt; i++)
      evt.p[i] = *p->args[i];
    return (insert_score_event(csound, &evt,
                               csound->sensEvents_state.curTime, 1) == 0 ?
            OK : NOTOK);
}

