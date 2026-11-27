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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "pfield.h"
#include "csoundCore.h"     /*                              LINEVENT.C      */
#include <ctype.h>

#ifdef MSVC
#include <fcntl.h>
#endif


#ifdef PIPES
# if defined(SGI) || defined(LINUX) || defined(NeXT) || defined(__MACH__)
#  define _popen popen
#  define _pclose pclose
# elif defined(__BEOS__) ||  defined(__HAIKU__) || defined(__MACH__)
#  include <stdio.h>
#  define _popen popen
#  define _pclose pclose
# else
   FILE *_popen(const char *, const char *);
# endif
#endif

#define LBUFSIZ1 32768
#define LF        '\n'

static void sense_line(CSOUND *csound, void *userData);
/* A reader is linked once it is complete: sensevents walks the list on the
   performance thread with no lock, and a host's first line links one from
   the host's thread. */
#if defined(MSVC)
#define PUBLISH(link, node) \
  _InterlockedExchangePointer((void *volatile *) &(link), (node))
#elif defined(HAVE_ATOMIC_BUILTIN)
#define PUBLISH(link, node) __atomic_store_n(&(link), (node), __ATOMIC_RELEASE)
#else
#define PUBLISH(link, node) ((link) = (node))
#endif

static int32_t set_sense_event_callback(CSOUND *csound, void (*func)(CSOUND *, void *),
                 void *userData)
{
  EVT_CB_FUNC *last = (EVT_CB_FUNC*) csound->evtFuncChain;
  EVT_CB_FUNC *fp = (EVT_CB_FUNC*) csound->Calloc(csound, sizeof(EVT_CB_FUNC));
  if (UNLIKELY(fp == NULL))
    return CSOUND_MEMORY;
  fp->func = func;
  fp->userData = userData;
  fp->nxt = NULL;
  if (last == NULL)
    PUBLISH(csound->evtFuncChain, (void *) fp);
  else {
    while (last->nxt != NULL)
      last = last->nxt;
    PUBLISH(last->nxt, fp);
  }
  csound->oparms->RTevents = 1;
  return 0;
}


#define STA(x)   (csound->lineventStatics.x)
#define MAXSTR 1048576 /* 1MB */

#ifndef O_NDELAY
#define O_NDELAY 0
#endif

static int32_t input_message(CSOUND *csound, const char *message,
                             int32_t block, int32_t *grown);

void linevent_open(CSOUND *csound)
/* set up Linebuf & ready the input files */
{ /*     callable once from musmon.c        */
    OPARMS  *O = csound->oparms;
    /* csound->lineventGlobals = (LINEVENT_GLOBALS*) */
    /*                            csound->Calloc(csound, */
    /*                            sizeof(LINEVENT_GLOBALS)); */

    int32_t grown = 0;

    /* The buffer, its position, the line reader and a block's buffer, made
       as a line sent makes them, under the lock: a line sent before the
       start has made them, and one sent from another thread now makes them,
       or finds them, once. An allocation that fails there fails the start,
       once the lock is let go, with no -L input to close. */
    if (UNLIKELY(input_message(csound, "", 1, &grown))) {
      O->Linein = 0;
      csound->LongJmp(csound, CSOUND_MEMORY);
    }
    if (strcmp(O->Linename, "stdin") == 0) {
      csound->Linefd = 0;
#if defined(DOSGCC) || defined(WIN32)
      setvbuf(stdin, NULL, _IONBF, 0);
      /* WARNING("-L stdin:  system has no fcntl function to get stdin"); */
#else
      STA(stdmode) = fcntl(csound->Linefd, F_GETFL, 0);
      if (UNLIKELY(fcntl(csound->Linefd, F_SETFL, STA(stdmode) | O_NDELAY) < 0))
        csoundDie(csound, Str("-L stdin fcntl failed\n"));
#endif
    }
#ifdef PIPES
    else if (UNLIKELY(O->Linename[0] == '|')) {
      csound->Linepipe = _popen(&(O->Linename[1]), "r");
      if (LIKELY(csound->Linepipe != NULL)) {
        csound->Linefd = fileno(csound->Linepipe);
        setvbuf(csound->Linepipe, NULL, _IONBF, 0);
      }
      else csoundDie(csound, Str("Cannot open %s\n"), O->Linename);
    }
#endif
#define MODE ,0
    else
      if (UNLIKELY((csound->Linefd=open(O->Linename, O_RDONLY|O_NDELAY MODE)) < 0))
        csoundDie(csound, Str("Cannot open %s\n"), O->Linename);
    if(csound->oparms->odebug)
    csound->Message(csound, Str("stdmode = %.8x Linefd = %d\n"),
                    STA(stdmode), csound->Linefd);
}

#ifdef PIPES
int32_t _pclose(FILE*);
#endif

void linevent_close(CSOUND *csound)
{
    if (csound->oparms->Linein == 0)
      return;
    csound->oparms->Linein = 0;
    if(csound->oparms->odebug)
    csound->Message(csound, Str("stdmode = %.8x Linefd = %d\n"),
                    STA(stdmode), csound->Linefd);
#ifdef PIPES
    if (csound->oparms->Linename[0] == '|')
      _pclose(csound->Linepipe);
    else
#endif
      {
        if (strcmp(csound->oparms->Linename, "stdin") != 0)
          close(csound->Linefd);
#if !defined(DOSGCC) && !defined(WIN32)
        else
          if (UNLIKELY(fcntl(csound->Linefd, F_SETFL, STA(stdmode))))
            csoundDie(csound, Str("Failed to set file status\n"));
#endif
      }

//csound->Free(csound, csound->lineventGlobals);
//csound->lineventGlobals = NULL;
}

/* does string segment contain LF? */

static inline int32_t containsLF(char *cp, const char *endp)
{
    while (cp < endp) {
      if (UNLIKELY(*cp++ == LF))
        return 1;
    }
    return 0;
}

static CS_NOINLINE int32_t linevent_alloc(CSOUND *csound, int32_t reallocsize)
{
    uint32_t tmp;

    if (reallocsize > 0) {
      /* VL 20-11-17 need to record the STA(Linep) offset
         in relation to STA(Linebuf) */
      tmp = (uint32_t) (STA(Linep) - STA(Linebuf));
      STA(Linebuf) = (char *) csound->ReAlloc(csound,
                                              (void *) STA(Linebuf), reallocsize);

      STA(linebufsiz) = reallocsize;
      STA(Linebufend) = STA(Linebuf) + STA(linebufsiz);
      /* VL 20-11-17 so we can place it in the correct position
         after reallocation */
      STA(Linep) =  STA(Linebuf) + tmp;
    } else if (STA(Linebuf)==NULL) {
       STA(linebufsiz) = LBUFSIZ1;
       STA(Linebuf) = (char *) csound->Calloc(csound, STA(linebufsiz));
    }
    if (STA(Linebuf) == NULL) {
       return 1;
    }
    //csound->Message(csound, "1. realloc: %d\n", reallocsize);
    if (STA(Linep)) return 0;
    csound->Linefd = -1;
    // allocate pfield memory
    if(STA(pfields) == NULL) {
     STA(msize) = PMAX;
     STA(pfields) = csound->Calloc(csound, sizeof(cs_float)*(STA(msize)+1));
    }
    STA(prve).opcod = ' ';
    STA(Linebufend) = STA(Linebuf) + STA(linebufsiz);
    set_sense_event_callback(csound, sense_line, NULL);
    /* last: the set-up is done once the reader is linked, and an allocation
       that fails before leaves it to the next line */
    STA(Linep) = STA(Linebuf);

    return 0;
}

/* insert text from an external source,
   to be interpreted as if coming in from stdin/Linefd for -L */

/* Whether a line of the message starts with a {, as an orchestra block's
   first line does. A { further on in a line, as in a string p-field, does
   not count, and a sorted score's lines never start with one. */
static int32_t starts_block(const char *line)
{
    while (line != NULL) {
      while (isblank((unsigned char) *line))
        line++;
      if (*line == '{')
        return 1;
      if ((line = strchr(line, LF)) != NULL)
        line++;
    }
    return 0;
}

/* The line-event buffer is appended to by whichever thread sends a message -
   a host's csoundEventString or csoundReadScore among them - and emptied by
   the performance thread's sense_line, at the top of each k-cycle: an append
   between the two was overwritten or cut off, or reallocated the buffer
   under the pointers sense_line held. Both take line_event_spinlock, and
   sense_line only for as long as it takes the complete lines out of the
   buffer (take_lines), so that what the lines then do - an orchestra block
   compiled, its global scorelinei appending again, a message callback - is
   never done under the lock, which is not recursive. The append's own
   messages wait for the lock to be let go too: they reach the host's
   callback, which may send a line itself. */
void csoundInputMessage(CSOUND *csound, const char *message) {
    int32_t grown = 0, lost;
    /* looked for before the lock, which the performance waits on */
    const int32_t block = starts_block(message);

    lost = input_message(csound, message, block, &grown);
    if (grown)
      csound->DebugMsg(csound, "realloc %d\n", grown);
    if (UNLIKELY(lost))
      csoundErrorMsg(csound, Str("LineBuffer Overflow - "
                                 "Input Data has been Lost\n"));
}

/* Appends message to the line-event buffer, under the lock, and so says
   nothing: what it grew the buffer by goes in *grown, and it returns 1 when
   the buffer could not grow and the message was lost. */
static int32_t append_message(CSOUND *csound, const char *message,
                              int32_t block, int32_t *grown) {
    int32  size = (int32) strlen(message);

    if (linevent_alloc(csound, 0) != 0) return 0;

    /* An orchestra block's buffer, made by the call that sends a { line,
       on its thread, rather than when sense_line reads the block */
    if (block && STA(orchestrab) == NULL)
      STA(orchestra) = STA(orchestrab) = (char *) csound->Calloc(csound, MAXSTR);
    if (!size) return 0;
    if (UNLIKELY((STA(Linep) + size) >= STA(Linebufend))) {
      /* and one more, for the newline added below */
      int32_t extralloc = (int32_t) (STA(Linep) + size + 1 - STA(Linebufend));
      if (linevent_alloc(csound, (STA(linebufsiz) + extralloc)) != 0)
        return 1;
      *grown = extralloc;
    }
    memcpy(STA(Linep), message, size);
    if (STA(Linep)[size - 1] != (char) '\n')
      STA(Linep)[size++] = (char) '\n';
    STA(Linep) += size;
    return 0;
}

/* append_message under line_event_spinlock, with a landing of this thread's
   own, set before the lock is taken: an allocation that fails in the append
   (memdie) lands there with the lock held, the message is lost, and the lock
   is let go as on the way out. The landing is this thread's (insert.c), so
   csound->exitjmp, which another API call may hold, is neither used nor set
   here. */
static int32_t input_message(CSOUND *csound, const char *message,
                             int32_t block, int32_t *grown) {
    API_CALL call;
    INIT_LANDING landing;
    volatile int32_t lost = 1;

    api_call_begin(csound, &call);
    init_landing_push(csound, &landing);
    if (setjmp(landing.target) == 0) {
      csoundSpinLock(&csound->line_event_spinlock);
      lost = append_message(csound, message, block, grown);
    }
    csoundSpinUnLock(&csound->line_event_spinlock);
    init_landing_pop(&landing);
    api_call_end(&call);
    return lost;
}

/* The complete lines waiting in the line-event buffer, with what the -L
   input has to give, moved into sense_line's own work buffer under the lock,
   and the rest - a line not ended yet - moved down to the buffer's start.
   Returns the end of what was taken, or NULL when no line is complete. */
static char *take_lines(CSOUND *csound)
{
    char    *cp, *last = NULL;
    int32_t len = 0, n;

    csoundSpinLock(&csound->line_event_spinlock);
    /* An init on the realtime init thread asked for a rewind, or the rewind
       it asked for is not made yet: the lines wait, and are read once the
       performance has rewound, as they are without --realtime. Under the
       lock: a line sent after the ask is never read before it. */
    if (ATOMIC_GET(csound->rewind_asking) != 0 ||
        ATOMIC_GET(csound->score_rewind_request) !=
        ATOMIC_GET(csound->score_rewind_served)) {
      csoundSpinUnLock(&csound->line_event_spinlock);
      return NULL;
    }
    if (csound->Linefd >= 0) {
      n = (int32_t) read(csound->Linefd, STA(Linep), STA(Linebufend) - STA(Linep));
      STA(Linep) += (n > 0 ? n : 0);
    }
    for (cp = STA(Linep); cp > STA(Linebuf); cp--)
      if (cp[-1] == LF) {
        last = cp;
        break;
      }
    if (last != NULL) {
      len = (int32_t) (last - STA(Linebuf));
      if (csound->line_event_work == NULL ||
          len + 1 > csound->line_event_work_size) {
        csound->line_event_work = (char *) (csound->line_event_work == NULL
                              ? csound->Calloc(csound, len + 1)
                              : csound->ReAlloc(csound, csound->line_event_work,
                                                len + 1));
        csound->line_event_work_size = len + 1;
      }
      memcpy(csound->line_event_work, STA(Linebuf), len);
      csound->line_event_work[len] = '\0';
      n = (int32_t) (STA(Linep) - last);
      memmove(STA(Linebuf), last, n);
      STA(Linep) = STA(Linebuf) + n;
    }
    csoundSpinUnLock(&csound->line_event_spinlock);
    return last == NULL ? NULL : csound->line_event_work + len;
}

/* accumlate RT Linein buffer, & place completed events in EVTBLK */
/* does more syntax checking than rdscor, since not preprocessed  */

static void sense_line(CSOUND *csound, void *userData)
{
    char    *cp, *Linestart, *Linend;
    int32_t     c, cpp1, n, pcnt, oflag = STA(oflag);
    IGN(userData);

    while (1) {
      if(STA(oflag) > oflag) break;
      if ((Linend = take_lines(csound)) == NULL)
        break;
      Linestart = csound->line_event_work;
      cp = Linestart;

      while (containsLF(Linestart, Linend)) {
        EVTBLK  e;
        cs_float *pfields = STA(pfields);
        char    *sstrp = NULL;
        int32_t     scnt = 0;
        int32_t     strsiz = 0;
        memset(&e, 0, sizeof(EVTBLK));
        memset(pfields, 0, STA(msize)*sizeof(cs_float));
        e.p = (cs_float *) pfields;
        e.strarg = NULL; e.scnt = 0;
        c = *cp;
        while (isblank(c))              /* skip initial white space */
          c = *(++cp);
        if (c == LF) {                  /* if null line, bugout     */
          Linestart = (++cp);
          continue;
        }
        cpp1 = *(cp+1);

        /* new orchestra input
         */
        if(STA(oflag)) {
          if(c == '}' && cpp1 != '}') {
            STA(oflag) = 0;
            if (STA(orchestra) != NULL) {
              csoundCompileOrc(csound, STA(orchestrab), 0);
              csound->Message(csound, Str("::compiling orchestra::\n"));
            }
            STA(orchestra) = STA(orchestrab);
            Linestart = (++cp);
            continue;
          }
          else {
            /* the block's text taken: to the last }, unless that ends a {{
               string, or all of it, with more to come */
            char *end = strrchr(Linestart, '}');
            if (end == NULL || *(end-1) == '}')
              end = Linend;
            if (STA(orchestra) != NULL &&
                end - Linestart >= MAXSTR - (STA(orchestra) - STA(orchestrab))) {
              csound->ErrorMsg(csound, Str("orchestra block longer than %d "
                                           "bytes: not compiled\n"),
                               MAXSTR - 1);
              STA(orchestra) = NULL;            /* its text dropped, to its } */
            }
            if (STA(orchestra) != NULL) {
              memcpy(STA(orchestra), Linestart, end - Linestart);
              STA(orchestra) += end - Linestart;
              *STA(orchestra) = '\0';
            }
            STA(oflag)++;
            if (end == Linend)
              Linestart = Linend;
            else
              cp = end;                         /* the } that ends it, next */
            continue;
          }
        } else if(c == '{') {
          STA(oflag) = 1;
          *STA(orchestrab) = '\0';             /* none of the block before */
          csound->Message(csound,
                          Str("::reading orchestra, use '}' to terminate::\n"));
          Linestart = (++cp);           /* the rest of the line is the block's */
          continue;
        }

        switch (c) {                    /* look for legal opcode    */
        case 'e':                       /* Quit realtime            */
        case 'i':
        case 'q':
        case 'f':
        case 'a':
        case 'd':
          e.opcod = c;
          break;
        default:
          csound->ErrorMsg(csound, Str("unknown opcode %c\n"), c);
          goto Lerr;
        }                                       /* for params that follow:  */
        pcnt = 0;
        do {
          char  *newcp;
          do {                                  /* skip white space */
            c = *(++cp);
          } while (isblank(c));
          if (c == LF)
            break;
          pcnt++;
          if (c == '"') {                       /* if find character string */
            char *quote = cp;
            if (e.strarg == NULL)
              e.strarg = csound->Malloc(csound, strsiz=SSTRSIZ);
            sstrp = e.strarg;
            n = scnt;
            while (n-->0) sstrp += strlen(sstrp)+1;
            n = 0;
            if (UNLIKELY(sstrp-e.strarg >= strsiz-10)) { /* as in rdscor */
              size_t start = (size_t) (sstrp-e.strarg);
              e.strarg = csound->ReAlloc(csound, e.strarg, strsiz+=SSTRSIZ);
              sstrp = e.strarg+start;
            }

            while ((c = *(++cp)) != '"') {
              /* VL: allow strings to be multi-line, within the lines taken */
              if (UNLIKELY(cp >= Linend)) {
                csound->ErrorMsg(csound, Str("unmatched quote\n"));
                cp = quote;
                goto Lerr;
              }

              if(c == '\\') {
                cp++;
                if(*cp == '"') c = *cp; /* if it is a double quote */
                /* otherwise we ignore it */
                else cp--;
              }
              sstrp[n++] = c;                   /*   save in private strbuf */

              if (UNLIKELY((sstrp-e.strarg)+n >= strsiz-10)) {
                size_t start = (size_t) (sstrp-e.strarg);
                e.strarg = csound->ReAlloc(csound, e.strarg, strsiz+=SSTRSIZ);
                sstrp = e.strarg+start;
              }
            }
            sstrp[n] = '\0';
            if (UNLIKELY(scnt >= CS_PFIELD_STRING_LIMIT)) goto Lerr;
            if (pcnt >= STA(msize)) {
              STA(msize) += PMAX;
              STA(pfields) = e.p = csound->ReAlloc(csound, e.p,
                                sizeof(cs_float) * (STA(msize) + 1));
            }
            e.p[pcnt] = csoundPFieldStringCode(scnt++);
            e.scnt = scnt;

            // printf("string: %s\n", sstrp);
            continue;
          }
          if (UNLIKELY(!(isdigit(c) || c == '+' || c == '-' || c == '.')))
            goto Lerr;
          if (c == '.' &&                       /*  if lone dot,       */
              (isblank(n = cp[1]) || n == LF)) {
            if (UNLIKELY(e.opcod != 'i' ||
                         STA(prve).opcod != 'i' || pcnt > STA(prve).pcnt)) {
              csound->ErrorMsg(csound, Str("dot carry has no reference\n"));
              goto Lerr;
            }                                   /*        pfld carry   */
            e.p[pcnt] = STA(prve).p[pcnt];
            if (UNLIKELY(IsStringCode(e.p[pcnt]))) {
              csound->ErrorMsg(csound, Str("cannot carry string p-field\n"));
              goto Lerr;
            }
            continue;
          }
          {
            cs_float tmpv = (cs_float) csoundStrtod(cp, &newcp);
            if (UNLIKELY(newcp == cp))          /* a sign or a dot, no number */
              goto Lerr;
            cp = newcp - 1;
            /* ensure capacity before writing */
            if (pcnt >= STA(msize)) {
              STA(msize) += PMAX;
              STA(pfields) = e.p = csound->ReAlloc(csound, e.p,
                                sizeof(cs_float) * (STA(msize) + 1));
            }
            e.p[pcnt] = tmpv;
          }

        } while (1);

        if (e.opcod =='f' && e.p[1]<FL(0.0)); /* an OK case */
        else  /* Check for sufficient pfields (0-based, opcode counted already). */
          if (UNLIKELY(pcnt < 2 && e.opcod != 'e')) {
            csound->ErrorMsg(csound, Str("too few pfields (%d)\n"), pcnt + 1);
            goto Lerr;
          }
        if (UNLIKELY(pcnt > 1 && e.p[2] < FL(0.0))) {
          csound->ErrorMsg(csound, Str("-L with negative p2 illegal\n"));
          goto Lerr;
        }
        e.pcnt = pcnt;                          /*   &  record pfld count    */
        if (e.opcod == 'i') {                   /* do carries for instr data */
          // free carry p-fields
          if(STA(prve).p != NULL) csound->Free(csound, STA(prve).p);
          // copy evtblk (except p-field pointer at the end)
          memcpy((void*) &STA(prve), (void*) &e, sizeof(EVTBLK) - sizeof(cs_float*));
          // allocate and copy new carry p-fields
          STA(prve).p = csound->Calloc(csound, sizeof(cs_float)*(e.pcnt+1));
          memcpy(STA(prve).p, e.p, sizeof(cs_float)*(e.pcnt+1));

          /* FIXME: how to carry string args ? */
          STA(prve).strarg = NULL;
        }
        if (UNLIKELY(pcnt >= PMAX && c != LF)) {
          csound->ErrorMsg(csound, Str("too many pfields"));
          while (*(++cp) != LF)                 /* flush any excess data     */
            ;
        }
        Linestart = (++cp);
        if (e.opcod =='e' && e.p[1]>FL(0.0)) {
          e.p[2] = e.p[1];
          e.pcnt = 2;
        }
        insert_event_at_sample(csound, &e, e.p+1, csound->icurTimeSamples);
        csound->Free(csound, e.strarg);         /* the event has its copy */
        continue;
      Lerr:
        csound->Free(csound, e.strarg);
        n = 0;                          /* error position, in its own line */
        while (cp - n > Linestart && cp[-n - 1] != LF)
          n++;
        while (*cp != LF)
          cp++;                                 /* go on to LF    */
        *cp = '\0';                             /*  & insert NULL */
        csound->ErrorMsg(csound, Str("illegal RT scoreline:\n%s\n%*s\n"),
                                 Linestart, n + 1, "^");  /* mark the error */
        Linestart = (++cp);
      }
      /* Every line taken ends with LF, so all were read: the next turn
         takes what has come since - from the -L input, from a host, or
         appended by what these lines did. */
    }

}






