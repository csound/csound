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
    char    *Linebuf, *Linep, *Linebufend;
    FILE    *Linecons;
    int     stdmode;
    EVTBLK  prve;
} LINEVENT_GLOBALS;

static void sensLine(CSOUND *csound, void *userData);

#define ST(x)   (((LINEVENT_GLOBALS*) ((CSOUND*) csound)->lineventGlobals)->x)

void RTLineset(CSOUND *csound)      /* set up Linebuf & ready the input files */
{                                   /*     callable once from musmon.c        */
    OPARMS  *O = csound->oparms;
    csound->lineventGlobals = (LINEVENT_GLOBALS*)
                               csound->Calloc(csound, sizeof(LINEVENT_GLOBALS));
    ST(prve).opcod = ' ';
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

/* insert text from an external source,
   to be interpreted as if coming in from stdin/Linefd for -L */

void writeLine(CSOUND *csound, const char *text, long size)
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

/* accumlate RT Linein buffer, & place completed events in EVTBLK */
/* does more syntax checking than rdscor, since not preprocessed  */

static void sensLine(CSOUND *csound, void *userData)
{
    int     c;
    char    *cp;
    int     n, pcnt;
    MYFLT   *fp;
    char    *Linend;

    while (csound->Linefd >= 0 &&
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
      if (containsLF(ST(Linebuf), Linend)) {
        EVTBLK  e;
        char    sstrp[SSTRSIZ];
        e.strarg = NULL;
        cp = ST(Linebuf);
        while ((c = *cp++) == ' ' || c == '\t') /* skip initial white space */
          ;
        if (c == LF) {                  /* if null line, bugout     */
          ST(Linep) = ST(Linebuf);
          while (cp < Linend)
            *ST(Linep)++ = *cp++;
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
          csound->Message(csound, Str("unknown opcode %c\n"), c);
          goto Lerr;
        }                                       /* for params that follow:  */
        for (fp = &(e.p[1]), pcnt = 1; c != LF && pcnt <= PMAX; pcnt++) {
          char *newcp, *s;
          while ((c = *cp++) == ' ' || c == '\t') /* skip white space */
            ;
          if (c == LF) break;
          if (c == '"') {                       /* if find character string */
            if (pcnt != 5 && (pcnt > 1 || e.opcod != 'i')) {
              /* (must be p5) */
              csound->Message(csound, Str("misplaced string\n"));
              goto Lerr;
            }
            e.strarg = s = &(sstrp[0]);
            s[0] = '\0';
            while ((c = *cp++) != '"') {
              if (c == LF) {
                csound->Message(csound, Str("unmatched quotes\n"));
                cp--;
                goto Lerr;
              }
              *s++ = c;                         /*   sav in private strbuf */
            }
            *s = '\0';
            *fp++ = SSTRCOD;                    /*   & store coded float   */
            continue;
          }
          if (!(isdigit(c) || c == '+' || c == '-' || c == '.'))
            goto Lerr;
          if (c == '.' &&                       /*  if lone dot,       */
              ((n = *cp) == ' ' || n == '\t' || n == LF)) {
            if (e.opcod != 'i' ||
                ST(prve).opcod != 'i' || pcnt > ST(prve).pcnt) {
              csound->Message(csound, Str("dot carry has no reference\n"));
              goto Lerr;
            }                                   /*        pfld carry   */
            *fp++ = ST(prve).p[pcnt];
            continue;
          }
          *fp++ = (MYFLT) strtod(cp - 1, &newcp);
          cp = newcp;
        }
        pcnt--;
        /* do carries for instr data */
        if (e.opcod == 'i') {
          /* FIXME: this could be done faster */
          memcpy(&ST(prve), &e, sizeof(EVTBLK));
          /* FIXME: how to carry string args ? */
          ST(prve).strarg = NULL;
        }
        if (pcnt < 3 && e.opcod != 'e') {       /* check sufficient pfields */
          csound->Message(csound, Str("too few pfields\n"));
          goto Lerr;
        }
        e.pcnt = pcnt;                          /*   &  record pfld count    */
        if (pcnt >= PMAX && c != LF) {
          csound->Message(csound, Str("too many pfields\n"));
          while (*cp++ != LF)                   /* flush any excess data     */
            ;
        }
        ST(Linep) = ST(Linebuf);
        while (cp < Linend)                     /* copy remaining data to    */
          *ST(Linep)++ = *cp++;                 /*     beginning of Linebuf  */
        if (e.p[2] < FL(0.0)) {
          csound->Message(csound, Str("-L with negative p2 illegal\n"));
          goto Lerr;
        }
        insert_score_event(csound, &e, csound->curTime);
      }
      else ST(Linep) += n;                      /* else just accum the chars */
      continue;

 Lerr:
      n = cp - ST(Linebuf) - 1;                 /* error position */
      while (*cp++ != LF);                      /* go on to LF    */
      *(cp - 1) = '\0';                         /*  & insert NULL */
      csound->Message(csound, Str("illegal RT scoreline:\n%s\n"), ST(Linebuf));
      while (n--)
        csound->Message(csound, " ");
      csound->Message(csound, "^\n");           /* mark the error */
      ST(Linep) = ST(Linebuf);
      while (cp < Linend)
        *ST(Linep)++ = *cp++;                   /* mov rem data forward */
    }
}

/* send a lineevent from the orchestra -matt 2001/12/07 */

static const char *errmsg_1 =
    "event: param 1 must be \"a\", \"i\", \"q\", \"f\", or \"e\"";
static const char *errmsg_2 =
    "event: string name is allowed only for \"i\" and \"q\" events";

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
        if (evt.opcod != 'i' && evt.opcod != 'q')
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
    if (insert_score_event(csound, &evt, csound->curTime) != 0)
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
    if ((opcod != 'a' && opcod != 'i' && opcod != 'q' && opcod != 'f' &&
         opcod != 'e') || ((char*) p->args[0])[1] != '\0')
      return csound->InitError(csound, Str(errmsg_1));
    evt.strarg = NULL;
    evt.opcod = opcod;
    evt.pcnt = p->INOCOUNT - 1;
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (p->XSTRCODE & 2) {
        if (evt.opcod != 'i' && evt.opcod != 'q')
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
      err = insert_score_event(csound, &evt, csound->curTime);
    if (err)
      csound->InitError(csound, Str("event_i: error creating '%c' event"),
                                opcod);
    return (err == 0 ? OK : NOTOK);
}

