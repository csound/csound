/*
    rtjack.c:

    Copyright (C) 2005 Istvan Varga

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

#include <jack/jack.h>
/* no #ifdef, should always have these on systems where JACK is available */
#include <unistd.h>
#include <stdint.h>
#include "csoundCore.h"
#include "csound.h"
#include "soundio.h"

#ifdef Str
#undef Str
#endif
#define Str(x) (((ENVIRON*) csound)->LocalizeString(x))

/* sample conversion routines */

static void float_to_float(int nSmps, float *inBuf, float *outBuf);
static void float_to_double(int nSmps, float *inBuf, double *outBuf);
static void double_to_float(int nSmps, double *inBuf, float *outBuf);
static void double_to_double(int nSmps, double *inBuf, double *outBuf);

/* module interface functions */

int csoundModuleCreate(void *csound)
{
    /* nothing to do, report success */
    ((ENVIRON*) csound)->Message(csound, "JACK real-time audio module "
                                         "for Csound by Istvan Varga\n");
    return 0;
}

static int playopen_(void*, csRtAudioParams*);
static int recopen_(void*, csRtAudioParams*);
static void rtplay_(void*, void*, int);
static int rtrecord_(void*, void*, int);
static void rtclose_(void*);

int csoundModuleInit(void *csound)
{
    ENVIRON *p;
    char    *drv;

    p = (ENVIRON*) csound;
    drv = (char*) (p->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "jack") == 0 || strcmp(drv, "Jack") == 0 ||
          strcmp(drv, "JACK") == 0))
      return 0;
    p->Message(csound, "rtaudio: JACK module enabled\n");
    p->SetPlayopenCallback(csound, playopen_);
    p->SetRecopenCallback(csound, recopen_);
    p->SetRtplayCallback(csound, rtplay_);
    p->SetRtrecordCallback(csound, rtrecord_);
    p->SetRtcloseCallback(csound, rtclose_);
    return 0;
}

/* open for audio input */

static int recopen_(void *csound, csRtAudioParams *parm)
{
    return -1;
}

/* open for audio output */

static int playopen_(void *csound, csRtAudioParams *parm)
{
    return -1;
}

/* get samples from ADC */

static int rtrecord_(void *csound, void *inbuf_, int bytes_)
{
    return 0;
}

/* put samples to DAC */

static void rtplay_(void *csound, void *outbuf_, int bytes_)
{
}

/* close the I/O device entirely  */
/* called only when both complete */

static void rtclose_(void *csound)
{
}

/* sample conversion routines */

static void float_to_float(int nSmps, float *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

static void float_to_double(int nSmps, float *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (double) *(inBuf++);
}

static void double_to_float(int nSmps, double *inBuf, float *outBuf)
{
    while (nSmps--)
      *(outBuf++) = (float) *(inBuf++);
}

static void double_to_double(int nSmps, double *inBuf, double *outBuf)
{
    while (nSmps--)
      *(outBuf++) = *(inBuf++);
}

