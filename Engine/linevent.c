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

#include "csoundCore.h"     /*                              LINEVENT.C      */
#include "text.h"
#include <ctype.h>
#if (defined(mac_classic) && defined(__MWERKS__)) || defined(SYMANTEC)
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

typedef struct {
    char    *Linep, *Linebufend;
    FILE    *Linecons;
    int     stdmode;
    EVTBLK  prve;
    char    Linebuf[LBUFSIZ];
} LINEVENT_GLOBALS;

static void sensLine(CSOUND *csound, void *userData);

#define ST(x)   (((LINEVENT_GLOBALS*) ((CSOUND*) csound)->lineventGlobals)->x)

void RTLineset(CSOUND *csound)      /* set up Linebuf & ready the input files */
{                                   /*     callable once from musmon.c        */
    OPARMS  *O = csound->oparms;
    csound->lineventGlobals = (LINEVENT_GLOBALS*)
                               csound->Calloc(csound, sizeof(LINEVENT_GLOBALS));
    ST(prve).opcod = ' ';
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
      if (UNLIKELY(fcntl(csound->Linefd, F_SETFL, ST(stdmode) | O_NDELAY) < 0))
        csoundDie(csound, Str("-L stdin fcntl failed"));
  #endif
#endif
    }
#ifdef PIPES
    else if (UNLIKELY(O->Linename[0] == '|')) {
      csound->Linepipe = _popen(&(O->Linename[1]), "r");
      if (LIKELY(csound->Linepipe != NULL)) {
        csound->Linefd = fileno(csound->Linepipe);
      }
      else csoundDie(csound, Str("Cannot open %s"), O->Linename);
    }
#endif
#if defined(mills_macintosh) || defined(SYMANTEC)
#define MODE
#else
#define MODE ,0
#endif
#if defined(MSVC)
#define O_RDONLY _O_RDONLY
#endif
    else
      if (UNLIKELY((csound->Linefd = open(O->Linename, O_RDONLY|O_NDELAY MODE)) < 0))
        csoundDie(csound, Str("Cannot open %s"), O->Linename);
    csound->Message(csound, Str("stdmode = %.8x Linefd = %d\n"),
                    ST(stdmode), csound->Linefd);
    csound->RegisterSenseEventCallback(csound, sensLine, NULL);
}

#ifdef PIPES
int _pclose(FILE*);
#endif

void RTclose(CSOUND *csound)
{
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
    else
  #endif
    {
      if (strcmp(csound->oparms->Linename, "stdin") != 0)
        close(csound->Linefd);
  #if !defined(DOSGCC) && !defined(__WATCOMC__) && !defined(WIN32) && \
      !defined(mills_macintosh)
      else
        fcntl(csound->Linefd, F_SETFL, ST(stdmode));
  #endif
    }
#endif      /* !(mills_macintosh || SYMANTEC) */
    csound->Free(csound, csound->lineventGlobals);
    csound->lineventGlobals = NULL;
}

/* does string segment contain LF? */

static inline int containsLF(char *cp, char *endp)
{
    while (cp < endp) {
      if (UNLIKELY(*cp++ == LF))
        return 1;
    }
    return 0;
}

static CS_NOINLINE int linevent_alloc(CSOUND *csound)
{
    volatile jmp_buf tmpExitJmp;
    int         err;

    csound->Linefd = -1;
    memcpy((void*) &tmpExitJmp, (void*) &csound->exitjmp, sizeof(jmp_buf));
    if ((err = setjmp(csound->exitjmp)) != 0) {
      memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));
      csound->lineventGlobals = NULL;
      return -1;
    }
    csound->lineventGlobals =
        (LINEVENT_GLOBALS*) mcalloc(csound, sizeof(LINEVENT_GLOBALS));
    memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));
    ST(prve).opcod = ' ';
    ST(Linebufend) = ST(Linebuf) + LBUFSIZ;
    ST(Linep) = ST(Linebuf);
    csound->RegisterSenseEventCallback(csound, sensLine, NULL);

    return 0;
}

/* insert text from an external source,
   to be interpreted as if coming in from stdin/Linefd for -L */

PUBLIC void csoundInputMessage(CSOUND *csound, const char *message)
{
    int32  size = (int32) strlen(message);

    if (!csound->lineventGlobals) {
      if (linevent_alloc(csound) != 0)
        return;
    }
    if (!size)
      return;
    if (UNLIKELY((ST(Linep) + size) >= ST(Linebufend))) {
      csoundErrorMsg(csound, Str("LineBuffer Overflow - "
                                 "Input Data has been Lost"));
      return;
    }
    memcpy(ST(Linep), message, size);
    if (ST(Linep)[size - 1] != (char) '\n')
      ST(Linep)[size++] = (char) '\n';
    ST(Linep) += size;
}

/* accumlate RT Linein buffer, & place completed events in EVTBLK */
/* does more syntax checking than rdscor, since not preprocessed  */

static void sensLine(CSOUND *csound, void *userData)
{
    char    *cp, *Linestart, *Linend;
    int     c, n, pcnt;

    while (1) {
      Linend = ST(Linep);
      if (csound->Linefd >= 0) {
#if defined(mills_macintosh) || defined(SYMANTEC)
        n = fread((void *) Linend, (size_t) 1,
                  (size_t) (ST(Linebufend) - Linend), ST(Linecons));
#else
        n = read(csound->Linefd, Linend, ST(Linebufend) - Linend);
#endif
        Linend += (n > 0 ? n : 0);
      }
      if (Linend <= ST(Linebuf))
        break;
      Linestart = ST(Linebuf);
      cp = Linestart;
      while (containsLF(Linestart, Linend)) {
        EVTBLK  e;
        char    sstrp[SSTRSIZ];
        e.strarg = NULL;
        c = *cp;
        while (c == ' ' || c == '\t')   /* skip initial white space */
          c = *(++cp);
        if (c == LF) {                  /* if null line, bugout     */
          Linestart = (++cp);
          continue;
        }
        switch (c) {                    /* look for legal opcode    */
        case 'e':                       /* Quit realtime            */
        case 'i':
        case 'q':
        case 'f':
        case 'a':
          e.opcod = c;
          break;
        default:
          csound->ErrorMsg(csound, Str("unknown opcode %c"), c);
          goto Lerr;
        }                                       /* for params that follow:  */
        pcnt = 0;
        do {
          char  *newcp;
          do {                                  /* skip white space */
            c = *(++cp);
          } while (c == ' ' || c == '\t');
          if (c == LF)
            break;
          pcnt++;
          if (c == '"') {                       /* if find character string */
            if (UNLIKELY(e.strarg != NULL)) {
              csound->ErrorMsg(csound, Str("multiple string p-fields"));
              goto Lerr;
            }
            n = 0;
            while ((c = *(++cp)) != '"') {
              if (UNLIKELY(c == LF)) {
                csound->ErrorMsg(csound, Str("unmatched quotes"));
                goto Lerr;
              }
              sstrp[n++] = c;                   /*   save in private strbuf */
              if (UNLIKELY(n >= SSTRSIZ)) {
                csound->ErrorMsg(csound, Str("string p-field is too long"));
                goto Lerr;
              }
            }
            sstrp[n] = '\0';
            e.strarg = &(sstrp[0]);
            e.p[pcnt] = SSTRCOD;                /*   & store coded float   */
            continue;
          }
          if (UNLIKELY(!(isdigit(c) || c == '+' || c == '-' || c == '.')))
            goto Lerr;
          if (c == '.' &&                       /*  if lone dot,       */
              ((n = cp[1]) == ' ' || n == '\t' || n == LF)) {
            if (UNLIKELY(e.opcod != 'i' ||
                         ST(prve).opcod != 'i' || pcnt > ST(prve).pcnt)) {
              csound->ErrorMsg(csound, Str("dot carry has no reference"));
              goto Lerr;
            }                                   /*        pfld carry   */
            e.p[pcnt] = ST(prve).p[pcnt];
            if (UNLIKELY(e.p[pcnt] == SSTRCOD)) {
              csound->ErrorMsg(csound, Str("cannot carry string p-field"));
              goto Lerr;
            }
            continue;
          }
          e.p[pcnt] = (MYFLT) strtod(cp, &newcp);
          cp = newcp - 1;
        } while (pcnt < PMAX);
        if (e.opcod =='f' && e.p[1]<FL(0.0)); /* an OK case */
        else        /* check sufficient pfields */
          if (UNLIKELY(pcnt < 3 && e.opcod != 'e')) {
            csound->ErrorMsg(csound, Str("too few pfields"));
            goto Lerr;
          }
        if (UNLIKELY(pcnt > 1 && e.p[2] < FL(0.0))) {
          csound->ErrorMsg(csound, Str("-L with negative p2 illegal"));
          goto Lerr;
        }
        e.pcnt = pcnt;                          /*   &  record pfld count    */
        if (e.opcod == 'i') {                   /* do carries for instr data */
          memcpy((void*) &ST(prve), (void*) &e,
                 (size_t) ((char*) &(e.p[pcnt + 1]) - (char*) &e));
          /* FIXME: how to carry string args ? */
          ST(prve).strarg = NULL;
        }
        if (UNLIKELY(pcnt >= PMAX && c != LF)) {
          csound->ErrorMsg(csound, Str("too many pfields"));
          while (*(++cp) != LF)                 /* flush any excess data     */
            ;
        }
        Linestart = (++cp);
        insert_score_event_at_sample(csound, &e, csound->icurTime);
        continue;
 Lerr:
        n = cp - Linestart;                     /* error position */
        while (*cp != LF)
          cp++;                                 /* go on to LF    */
        *cp = '\0';                             /*  & insert NULL */
        csound->ErrorMsg(csound, Str("illegal RT scoreline:\n%s\n%*s"),
                                 Linestart, n + 1, "^");  /* mark the error */
        Linestart = (++cp);
      }
      if (Linestart != &(ST(Linebuf)[0])) {
        int len = (int) (Linend - Linestart);
        /* move any remaining characters to the beginning of the buffer */
        for (n = 0; n < len; n++)
          ST(Linebuf)[n] = Linestart[n];
        n = (int) (Linestart - &(ST(Linebuf)[0]));
        ST(Linep) -= n;
        Linend -= n;
      }
      if (Linend == ST(Linep))      /* return if no more data is available  */
        break;
      ST(Linep) = Linend;                       /* accum the chars          */
    }
}

/* send a lineevent from the orchestra -matt 2001/12/07 */

static const char *errmsg_1 =
  Str_noop("event: param 1 must be \"a\", \"i\", \"q\", \"f\", or \"e\"");
static const char *errmsg_2 =
  Str_noop("event: string name is allowed only for \"i\" and \"q\" events");

int eventOpcode(CSOUND *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i;
    char    opcod;

    opcod = ((char*) p->args[0])[0];
    if ((opcod != 'a' && opcod != 'i' && opcod != 'q' && opcod != 'f' &&
         opcod != 'e') || ((char*) p->args[0])[1] != '\0')
      return csound->PerfError(csound, Str(errmsg_1));
    evt.strarg = NULL;
    evt.opcod = opcod;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (p->XSTRCODE & 2) {
        if (UNLIKELY(evt.opcod != 'i' && evt.opcod != 'q'))
          return csound->PerfError(csound, Str(errmsg_2));
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
    if (insert_score_event_at_sample(csound, &evt, csound->icurTime) != 0)
      return csound->PerfError(csound, Str("event: error creating '%c' event"),
                                       opcod);
    return OK;
}

/* i-time version of event opcode */

int eventOpcodeI(CSOUND *csound, LINEVENT *p)
{
    EVTBLK  evt;
    int     i, err = 0;
    char    opcod;

    opcod = ((char*) p->args[0])[0];
    if (UNLIKELY((opcod != 'a' && opcod != 'i' && opcod != 'q' && opcod != 'f' &&
                  opcod != 'e') || ((char*) p->args[0])[1] != '\0'))
      return csound->InitError(csound, Str(errmsg_1));
    evt.strarg = NULL;
    evt.opcod = opcod;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (p->XSTRCODE & 2) {
        if (UNLIKELY(evt.opcod != 'i' && evt.opcod != 'q'))
          return csound->InitError(csound, Str(errmsg_2));
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
    if (opcod == 'f' && (int) evt.pcnt >= 2 && evt.p[2] <= FL(0.0)) {
      FUNC  *dummyftp;
      err = csound->hfgens(csound, &dummyftp, &evt, 0);
    }
    else
      err = insert_score_event_at_sample(csound, &evt, csound->icurTime);
    if (UNLIKELY(err))
      csound->InitError(csound, Str("event_i: error creating '%c' event"),
                                opcod);
    return (err == 0 ? OK : NOTOK);
}

