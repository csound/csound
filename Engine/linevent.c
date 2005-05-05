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
#include <ctype.h>
#if defined(mills_macintosh) || defined(SYMANTEC)
#include <console.h>
#endif

#include "linevent.h"

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

typedef struct levtblk {
    struct levtblk *nxtblk;
    struct levtblk *nxtact;
    long      inuse, oncounter;
    EVTBLK    evtblk;
} LEVTBLK;

typedef struct {
    char    *Linebuf, *Linep, *Linebufend;
    FILE    *Linecons;
    LEVTBLK *Firstblk;
    LEVTBLK *Firstact;
    int     stdmode;
    EVTBLK  *prve;
    char    prvop /* = ' ' */;
    int     prvpcnt;
    MYFLT   prvp1 /* = FL(0.0) */;
    MYFLT   prvp2 /* = FL(0.0) */;
} LINEVENT_GLOBALS;

#define ST(x)   (((LINEVENT_GLOBALS*) ((ENVIRON*) csound)->lineventGlobals)->x)

void RTLineset(ENVIRON *csound)     /* set up Linebuf & ready the input files */
{                                   /*     callable once from musmon.c        */
    OPARMS  *O = csound->oparms;
    csound->lineventGlobals = (LINEVENT_GLOBALS*)
                               csound->Calloc(csound, sizeof(LINEVENT_GLOBALS));
    ST(prvop) = ' ';
    ST(prvp1) = FL(0.0);
    ST(prvp2) = FL(0.0);
    ST(Firstblk) = (LEVTBLK *) mcalloc(csound, sizeof(LEVTBLK));
    ST(Firstact) = NULL;
    ST(Linebuf) = mcalloc(csound, LBUFSIZ);
    ST(Linebufend) = ST(Linebuf) + LBUFSIZ;
    ST(Linep) = ST(Linebuf);
    if (strcmp(O->Linename, "stdin") == 0) {
#ifdef SYMANTEC
      console_options.top += 10;
      console_options.left += 10;
      console_options.title = "\pRT Line_events";
      console_options.nrows = 10;
      console_options.ncols = 50;
      ST(Linecons) = fopenc();
      cshow(ST(Linecons));
#elif defined(mills_macintosh)
      ST(Linecons) = stdin;
      setvbuf(stdin, NULL, _IONBF, 0);
#else
  #if defined(DOSGCC) || defined(__WATCOMC__) || defined(WIN32) || \
      defined(mills_macintosh)
      setvbuf(stdin, NULL, _IONBF, 0);
   /* WARNING("-L stdin:  system has no fcntl function to get stdin"); */
  #else
      ST(stdmode) = fcntl(csound->Linefd, F_GETFL, 0);
      if (fcntl(csound->Linefd, F_SETFL, ST(stdmode) | O_NDELAY) < 0)
        csoundDie(csound, Str("-L stdin fcntl failed"));
  #endif
#endif
    }
#ifdef PIPES
    else if (O->Linename[0] == '|') {
      csound->Linepipe = _popen(&(O->Linename[1]), "r");
      if (csound->Linepipe == NULL) {
        FILE *xxx = csound->Linepipe;
        csound->Linefd = fileno(xxx);
      }
      else csoundDie(csound, Str("Cannot open %s"), O->Linename);
    }
#endif
#if defined(mills_macintosh) || defined(SYMANTEC)
#define MODE
#else
#define MODE ,0
#endif
    else if ((csound->Linefd = open(O->Linename, O_RDONLY|O_NDELAY  MODE)) < 0)
      csoundDie(csound, Str("Cannot open %s"), O->Linename);
    csound->Message(csound, Str("stdmode = %.8x Linefd = %d\n"),
                            ST(stdmode), csound->Linefd);
}

#ifdef PIPES
int _pclose(FILE*);
#endif

void RTclose(void *csound_)
{
    ENVIRON *csound = (ENVIRON*) csound_;

    if (csound->oparms->Linein == 0 || csound->lineventGlobals == NULL)
      return;
    csound->oparms->Linein = 0;
    csound->Message(csound, Str("stdmode = %.8x Linefd = %d\n"),
                            ST(stdmode), csound->Linefd);
#if defined(mills_macintosh) || defined(SYMANTEC)
    if (ST(Linecons) != NULL)
      fclose(ST(Linecons));
#else
  #ifdef PIPES
    if (csound->oparms->Linename[0] == '|')
      _pclose(csound->Linepipe);
  #endif
    else {
      if (strcmp(csound->oparms->Linename, "stdin") != 0)
        close(csound->Linefd);
  #if !defined(DOSGCC) && !defined(__WATCOMC__) && !defined(WIN32) && \
      !defined(mills_macintosh)
      else
        fcntl(csound->Linefd, F_SETFL, ST(stdmode));
  #endif
    }
#endif      /* !(mills_macintosh || SYMANTEC) */
    if (ST(Linebuf) != NULL)
      mfree(csound, ST(Linebuf));
    csound->Free(csound, csound->lineventGlobals);
    csound->lineventGlobals = NULL;
}

static int containsLF(char *cp, char *endp)/* does string segment contain LF? */
{
    do {
      if (*cp++ == LF)  return(1);
    } while (cp < endp);
    return(0);
}

static LEVTBLK *getblk(ENVIRON *csound)
{                       /* get blk from the LEVTBLK pool, or alloc a new one */
    LEVTBLK *curp = ST(Firstblk), *nxtp;

    while (curp->inuse) {
      if ((nxtp = curp->nxtblk) == NULL) {
        nxtp = (LEVTBLK *) mcalloc(csound, sizeof(LEVTBLK));
        curp->nxtblk = nxtp;
      }
      curp = nxtp;
    }
    return(curp);
}

static void Linsert(ENVIRON *csound, LEVTBLK *curp)
{                       /* insert blk into the time-ordered linevent queue */
    LEVTBLK *nxtp = ST(Firstact), *prvp = NULL;

    while (nxtp != NULL && nxtp->inuse && nxtp->oncounter <= curp->oncounter) {
      prvp = nxtp;
      nxtp = nxtp->nxtact;
    }
    if (prvp == NULL)
        ST(Firstact) = curp;
    else prvp->nxtact = curp;
    curp->nxtact = nxtp;
}

/* insert text from an external source, to be interpreted as if
    coming in from stdin/Linefd for -L*/
void writeLine(ENVIRON *csound, const char *text, long size)
{
    if (ST(Linebuf)) {
      if ((ST(Linep) + size) < ST(Linebufend)) {
        memcpy(ST(Linep), text, size);
        ST(Linep) += size;
      }
      else {
        csound->Warning(csound, Str("LineBuffer Overflow - "
                                    "Input Data has been Lost"));
      }
    }
    else {
      csound->Warning(csound, Str("Input ignored, RT Line Events "
                                  "(-L) has not been initialised"));
    }
}

int sensLine(ENVIRON *csound)
    /* accumlate RT Linein buffer, & place completed events in EVTBLK */
{   /* does more syntax checking than rdscor, since not preprocessed  */
    int     c;
    char    *cp;
    LEVTBLK *Curblk;
    EVTBLK  *e;
    int     n, pcnt;
    MYFLT   *fp, *prvfp;
    char    *Linend;

    if (csound->Linefd >= 0 &&
        (
#if defined(mills_macintosh) || defined(SYMANTEC)
         ((n = fread((void *) ST(Linep), (size_t) 1,
                     (size_t) (ST(Linebufend) - ST(Linep)),
                     ST(Linecons))) > 0) ||
#else
         ((n = read(csound->Linefd, ST(Linep),
                    ST(Linebufend) - ST(Linep))) > 0) ||
#endif
         ST(Linep) > ST(Linebuf)))
      {
        Linend = ST(Linep) + (n > 0 ? n : 0);
  /*    if ((c = *(Linend - 1)) == LF || containsLF(ST(Linep),Linend)) { */
        if (containsLF(ST(Linebuf), Linend)) {
          cp = ST(Linebuf);
          Curblk = getblk(csound);      /* get a blk from the levtblk pool */
          e = &Curblk->evtblk;
          while ((c = *cp++) == ' ' || c == '\t')/* skip initial white space */
            ;
          if (c == LF) goto Timchk;     /* if null line, bugout     */
          switch (c) {                  /* look for legal opcode    */
          case 'e':                     /* Quit realtime            */
          case 'i':
          case 'q':
          case 'f':
            e->opcod = c;
            break;
          default:
            csound->Message(csound, Str("unknown opcode %c\n"));
            goto Lerr;
          }                                     /* for params that follow:  */
          for (fp = &e->p[1], pcnt = 0; c != LF && pcnt<PMAX; pcnt++) {
        /*  long val = 0, scale = 1;  */
            char *newcp;
            while ((c = *cp++) == ' ' || c == '\t') /* skip white space */
              ;
            if (c == LF) break;
            if (c == '"') {                     /* if find character string  */
              char *sstrp;      /* IV - Oct 31 2002: allow string instrname */
              if (pcnt != 4 && (pcnt || e->opcod != 'i')) {
                /* (must be p5) */
                csound->Message(csound, Str("misplaced string\n"));
                goto Lerr;
              }
              if ((sstrp = e->strarg) == NULL)
                e->strarg = sstrp = mcalloc(csound, SSTRSIZ);
              while ((c = *cp++) != '"') {
                if (c == LF) {
                  csound->Message(csound, Str("unmatched quotes\n"));
                  cp--;
                  goto Lerr;
                }
                *sstrp++ = c;                   /*   sav in private strbuf */
              }
              *sstrp = '\0';
              *fp++ = SSTRCOD;                  /*   & store coded float   */
              continue;
            }
            if (!(isdigit(c) || c=='+' || c=='-' || c=='.'))
              goto Lerr;
            if (c == '.'                        /*  if lone dot,       */
                && ((n = *cp)==' ' || n=='\t' || n==LF)) {
              if (e->opcod != 'i' || ST(prvop) != 'i' || pcnt >= ST(prvpcnt)) {
                csound->Message(csound, Str("dot carry has no reference\n"));
                goto Lerr;
              }
              if (e != ST(prve)) {              /*        pfld carry   */
                if (pcnt == 1)
                  *fp = ST(prvp2);
                else *fp = ST(prve)->p[pcnt+1];
              }
              fp++;
              continue;
            }
            *fp++ = (MYFLT)strtod(cp-1, &newcp);
            cp = newcp;
          }
          if (e->opcod == 'i' && ST(prvop) == 'i')
            /* do carries for instr data */
            if (!pcnt || (pcnt < ST(prvpcnt) && e->p[1] == ST(prvp1))) {
              int pcntsav = pcnt;
              if (e != ST(prve))
                for (prvfp = &ST(prve)->p[pcnt+1]; pcnt < ST(prvpcnt); pcnt++)
                  *fp++ = *prvfp++;
              else pcnt =  ST(prvpcnt);
              if (pcntsav < 2)
                e->p[2] = ST(prvp2);
            }
          if (pcnt < 3 && e->opcod != 'e') {    /* check sufficient pfields */
            csound->Message(csound, Str("too few pfields\n"));
            goto Lerr;
          }
          e->pcnt = pcnt;                       /*   &  record pfld count    */
          ST(prve) = e;
          ST(prvop) = e->opcod;                 /* preserv the carry sensors */
          ST(prvpcnt) = pcnt;
          ST(prvp1) = e->p[1];
          ST(prvp2) = e->p[2];
          if (pcnt == PMAX && c != LF) {
            csound->Message(csound, Str("too many pfields\n"));
            while (*cp++ != LF)                 /* flush any excess data     */
              ;
          }
          ST(Linep) = ST(Linebuf);
          while (cp < Linend)                   /* copy remaining data to    */
            *ST(Linep)++ = *cp++;               /*     beginning of Linebuf  */
          if (e->p[2] < 0.) {
            csound->Message(csound, Str("-L with negative p2 illegal\n"));
            goto Lerr;
          }
          e->p2orig = e->p[2];
          e->p3orig = e->p[3];
          Curblk->inuse = 1;
          Curblk->oncounter = csound->global_kcounter
                              + (long) (e->p[2] * csound->global_ekr);
          Linsert(csound, Curblk);
        }
        else ST(Linep) += n;                    /* else just accum the chars */
      }

 Timchk:
    if (ST(Firstact) != NULL &&                 /* if an event is due now,   */
        ST(Firstact)->oncounter <= csound->global_kcounter) {
      csound->Linevtblk = &ST(Firstact)->evtblk;
      ST(Firstact)->inuse = 0;                  /*  mark its space available */
      ST(Firstact) = ST(Firstact)->nxtact;
      return(1);                                /* & report Line-type RTevent */
    }
    else return(0);                             /* else nothing due yet      */

 Lerr:
    n = cp - ST(Linebuf) - 1;                   /* error position */
    while (*cp++ != LF);                        /* go on to LF    */
    *(cp-1) = '\0';                             /*  & insert NULL */
    csound->Message(csound, Str("illegal RT scoreline:\n%s\n"), ST(Linebuf));
    while (n--)
      csound->Message(csound, " ");
    csound->Message(csound, "^\n");             /* mark the error */
    ST(Linep) = ST(Linebuf);
    while (cp < Linend)
      *ST(Linep)++ = *cp++;                     /* mov rem data forward */
    return(0);
}

/* send a lineevent from the orchestra */
/* code derived from sensLine() -matt 2001/12/07 */

int eventOpcode(ENVIRON *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i;
    char    opcod;

    if (!(p->XSTRCODE & 1) ||
        ((opcod = ((char*) p->args[0])[0]) != 'i' && opcod != 'q' &&
         opcod != 'f' && opcod != 'e'))
      return csound->PerfError(csound, Str("event param 1 must be "
                                           "\"i\", \"q\", \"f\", or \"e\""));
    evt.strarg = NULL;
    evt.opcod = opcod;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (p->XSTRCODE & 2) {
        if (evt.opcod != 'i' && evt.opcod != 'q')
          return
            csound->PerfError(csound, Str("event: string name is allowed "
                                          "only for \"i\" and \"q\" events"));
        evt.p[1] = SSTRCOD;
        evt.strarg = (char*) p->args[1];
      }
      else {
        evt.p[1] = *p->args[1];
        evt.strarg = NULL;
      }
      for (i = 2; i <= evt.pcnt; i++)
        evt.p[i] = *p->args[i];
    }
    return (insert_score_event(csound, &evt,
                               csound->sensEvents_state.curTime, 0) == 0 ?
            OK : NOTOK);
}

/* i-time version of event opcode */

int eventOpcodeI(ENVIRON *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i;
    char    opcod;

    if (!(p->XSTRCODE & 1) ||
        ((opcod = ((char*) p->args[0])[0]) != 'i' && opcod != 'q' &&
         opcod != 'f' && opcod != 'e'))
      return csound->InitError(csound, Str("event param 1 must be "
                                           "\"i\", \"q\", \"f\", or \"e\""));
    evt.strarg = NULL;
    evt.opcod = opcod;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (p->XSTRCODE & 2) {
        if (evt.opcod != 'i' && evt.opcod != 'q')
          return
            csound->InitError(csound, Str("event: string name is allowed "
                                          "only for \"i\" and \"q\" events"));
        evt.p[1] = SSTRCOD;
        evt.strarg = (char*) p->args[1];
      }
      else {
        evt.p[1] = *p->args[1];
        evt.strarg = NULL;
      }
      for (i = 2; i <= evt.pcnt; i++)
        evt.p[i] = *p->args[i];
    }
    return (insert_score_event(csound, &evt,
                               csound->sensEvents_state.curTime, 1) == 0 ?
            OK : NOTOK);
}

