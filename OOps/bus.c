/*
    bus.c:

    Copyright (C) 2004 John ffitch
              (C) 2005 Istvan Varga

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
                                /*                      BUS.C           */
#include "csoundCore.h"
#include <setjmp.h>
#include "aops.h"

static CS_NOINLINE int chan_realloc(CSOUND *csound,
                                    MYFLT **p, int *oldSize, int newSize)
{
    volatile jmp_buf  saved_exitjmp;
    MYFLT             *newp;
    int               i;

    memcpy((void*) &saved_exitjmp, (void*) &csound->exitjmp, sizeof(jmp_buf));
    if (setjmp(csound->exitjmp) != 0) {
      memcpy((void*) &csound->exitjmp, (void*) &saved_exitjmp, sizeof(jmp_buf));
      return CSOUND_MEMORY;
    }
    newp = (MYFLT*) mrealloc(csound, (void*) (*p), sizeof(MYFLT) * newSize);
    memcpy((void*) &csound->exitjmp, (void*) &saved_exitjmp, sizeof(jmp_buf));
    i = (*oldSize);
    do {
      newp[i] = FL(0.0);
    } while (++i < newSize);
    (*p) = newp;
    (*oldSize) = newSize;
    return CSOUND_SUCCESS;
}

/**
 * Sends a MYFLT value to the chani opcode (k-rate) at index 'n'.
 * The bus is automatically extended if 'n' exceeds any previously used
 * index value, clearing new locations to zero.
 * Returns zero on success, CSOUND_ERROR if the index is invalid, and
 * CSOUND_MEMORY if there is not enough memory to extend the bus.
 */
PUBLIC int csoundChanIKSet(CSOUND *csound, MYFLT value, int n)
{
    if ((unsigned int) n >= (unsigned int) csound->nchanik) {
      int   err;
      if (n < 0)
        return CSOUND_ERROR;
      err = chan_realloc(csound, &(csound->chanik), &(csound->nchanik), n + 1);
      if (err)
        return err;
    }
    csound->chanik[n] = value;
    return CSOUND_SUCCESS;
}

/**
 * Receives a MYFLT value from the chano opcode (k-rate) at index 'n'.
 * The bus is automatically extended if 'n' exceeds any previously used
 * index value, clearing new locations to zero.
 * Returns zero on success, CSOUND_ERROR if the index is invalid, and
 * CSOUND_MEMORY if there is not enough memory to extend the bus.
 */
PUBLIC int csoundChanOKGet(CSOUND *csound, MYFLT *value, int n)
{
    if ((unsigned int) n >= (unsigned int) csound->nchanok) {
      int   err;
      if (n < 0)
        return CSOUND_ERROR;
      err = chan_realloc(csound, &(csound->chanok), &(csound->nchanok), n + 1);
      if (err)
        return err;
    }
    (*value) = csound->chanok[n];
    return CSOUND_SUCCESS;
}

/**
 * Sends ksmps MYFLT values to the chani opcode (a-rate) at index 'n'.
 * The bus is automatically extended if 'n' exceeds any previously used
 * index value, clearing new locations to zero.
 * Returns zero on success, CSOUND_ERROR if the index is invalid, and
 * CSOUND_MEMORY if there is not enough memory to extend the bus.
 */
PUBLIC int csoundChanIASet(CSOUND *csound, const MYFLT *value, int n)
{
    n *= csound->ksmps;
    if ((unsigned int) n >= (unsigned int) csound->nchania) {
      int   err;
      if (n < 0)
        return CSOUND_ERROR;
      err = chan_realloc(csound, &(csound->chania),
                         &(csound->nchania), n + csound->ksmps);
      if (err)
        return err;
    }
    memcpy(&(csound->chania[n]), value, sizeof(MYFLT) * csound->ksmps);
    return CSOUND_SUCCESS;
}

/**
 * Receives ksmps MYFLT values from the chano opcode (a-rate) at index 'n'.
 * The bus is automatically extended if 'n' exceeds any previously used
 * index value, clearing new locations to zero.
 * Returns zero on success, CSOUND_ERROR if the index is invalid, and
 * CSOUND_MEMORY if there is not enough memory to extend the bus.
 */
PUBLIC int csoundChanOAGet(CSOUND *csound, MYFLT *value, int n)
{
    n *= csound->ksmps;
    if ((unsigned int) n >= (unsigned int) csound->nchanoa) {
      int   err;
      if (n < 0)
        return CSOUND_ERROR;
      err = chan_realloc(csound, &(csound->chanoa),
                         &(csound->nchanoa), n + csound->ksmps);
      if (err)
        return err;
    }
    memcpy(value, &(csound->chanoa[n]), sizeof(MYFLT) * csound->ksmps);
    return CSOUND_SUCCESS;
}

 /* ------------------------------------------------------------------------ */

#ifdef HAVE_C99
#  ifdef MYFLT2LRND
#    undef MYFLT2LRND
#  endif
#  ifndef USE_DOUBLE
#    define MYFLT2LRND  lrintf
#  else
#    define MYFLT2LRND  lrint
#  endif
#endif

int chani_opcode_perf_k(CSOUND *csound, ASSIGN *p)
{
    int     n = (int) MYFLT2LRND(*(p->a));

    if ((unsigned int) n >= (unsigned int) csound->nchanik) {
      if (n < 0)
        return csound->PerfError(csound, Str("chani: invalid index"));
      if (chan_realloc(csound, &(csound->chanik),
                       &(csound->nchanik), n + 1) != 0)
        return csound->PerfError(csound,
                                 Str("chani: memory allocation failure"));
    }
    *(p->r) = csound->chanik[n];
    return OK;
}

int chano_opcode_perf_k(CSOUND *csound, ASSIGN *p)
{
    int     n = (int) MYFLT2LRND(*(p->a));

    if ((unsigned int) n >= (unsigned int) csound->nchanok) {
      if (n < 0)
        return csound->PerfError(csound, Str("chano: invalid index"));
      if (chan_realloc(csound, &(csound->chanok),
                       &(csound->nchanok), n + 1) != 0)
        return csound->PerfError(csound,
                                 Str("chano: memory allocation failure"));
    }
    csound->chanok[n] = *(p->r);
    return OK;
}

int chani_opcode_perf_a(CSOUND *csound, ASSIGN *p)
{
    int     n = (int) MYFLT2LRND(*(p->a)) * csound->global_ksmps;

    if ((unsigned int) n >= (unsigned int) csound->nchania) {
      if (n < 0)
        return csound->PerfError(csound, Str("chani: invalid index"));
      if (chan_realloc(csound, &(csound->chania),
                       &(csound->nchania), n + csound->global_ksmps) != 0)
        return csound->PerfError(csound,
                                 Str("chani: memory allocation failure"));
    }
    memcpy(p->r, &(csound->chania[n]), sizeof(MYFLT) * csound->ksmps);
    return OK;
}

int chano_opcode_perf_a(CSOUND *csound, ASSIGN *p)
{
    int     n = (int) MYFLT2LRND(*(p->a)) * csound->global_ksmps;

    if ((unsigned int) n >= (unsigned int) csound->nchanoa) {
      if (n < 0)
        return csound->PerfError(csound, Str("chano: invalid index"));
      if (chan_realloc(csound, &(csound->chanoa),
                       &(csound->nchanoa), n + csound->global_ksmps) != 0)
        return csound->PerfError(csound,
                                 Str("chano: memory allocation failure"));
    }
    memcpy(&(csound->chanoa[n]), p->r, sizeof(MYFLT) * csound->ksmps);
    return OK;
}

/* ********************************************************************** */
/* *************** SENSING ********************************************** */
/* ********************************************************************** */

#if defined(__unix) || defined(__unix__) || defined(__MACH__)
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif
#  ifdef HAVE_TERMIOS_H
#    include <termios.h>
#  endif

int sensekey_init(CSOUND *csound, KSENSE *p)
{
    if (csound->inChar_ < 0) {
#  if defined(WIN32)
      setvbuf(stdin, NULL, _IONBF, 0);  /* Does not seem to work */
#  elif defined(HAVE_TERMIOS_H)
      struct termios  tty;
      tcgetattr(0, &tty);
      tty.c_lflag &= (~ICANON);
      tcsetattr(0, TCSANOW, &tty);
#  endif
    }
    return OK;
}

int sensekey_perf(CSOUND *csound, KSENSE *p)
{
    fd_set          rfds;
    struct timeval  tv;
    int             retval;

    if (csound->inChar_ < 0) {
      /* Watch stdin (fd 0) to see when it has input. */
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);
      /* No waiting */
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      retval = select(1, &rfds, NULL, NULL, &tv);

      if (retval) {
        char  ch;
        read(0, &ch, 1);
        *p->ans = (MYFLT) ch;
     /* FD_ISSET(0, &rfds) will be true. */
      }
      else
        *p->ans = FL(-1.0);
    }
    else {
      if (csound->inChar_) {
        *p->ans = (MYFLT) csound->inChar_;
        csound->inChar_ = 0;
      }
      else
        *p->ans = FL(-1.0);
    }

    return OK;
}

#else       /* __unix || __unix__ || __MACH__ */

int sensekey_init(CSOUND *csound, KSENSE *p)
{
    (void) csound;
    (void) p;
    return OK;
}

#  ifdef WIN32
#    include <conio.h>

int sensekey_perf(CSOUND *csound, KSENSE *p)
{
    if (csound->inChar_ < 0) {
      if (_kbhit())
        *p->ans = (MYFLT) _getch();
      else
        *p->ans = FL(-1.0);
    }
    else {
      if (csound->inChar_) {
        *p->ans = (MYFLT) csound->inChar_;
        csound->inChar_ = 0;
      }
      else
        *p->ans = FL(-1.0);
    }

    return OK;
}

#  else     /* WIN32 */

int sensekey_perf(CSOUND *csound, KSENSE *p)
{
    if (csound->inChar_ < 0) {
      *p->ans = (MYFLT) getchar();
    }
    else {
      if (csound->inChar_) {
        *p->ans = (MYFLT) csound->inChar_;
        csound->inChar_ = 0;
      }
      else
        *p->ans = FL(-1.0);
    }

    return OK;
}

#  endif    /* !WIN32 */
#endif      /* !(__unix || __unix__ || __MACH__) */

