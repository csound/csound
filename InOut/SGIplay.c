/*  
    SGIplay.c:

    Copyright (C) 1991 Barry Vercoe, Dan Ellis

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

/***************************************************************\
*       SGIplay.c                                               *
*  provide lofi-style device interface for realtime output on   *
*  SGI Iris Indigo hardware                                     *
*  dpwe 12nov91                                                 *
\***************************************************************/

#include <audio.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
static ALport playPort = 0L;
static ALport recPort = 0L;
static int pldszstat = 0;
static int rcdszstat = 0;
static int nchans = 0;

#define NUM_PARAMS 1
void play_set(int chans, int dsize, MYFLT srate, int scale)
{
    ALconfig config;
    long params[2*NUM_PARAMS];

    pldszstat = dsize;  /* remember what it is set to */
    config = ALnewconfig();
    nchans = chans;     /* static global for play_on */
    ALsetchannels(config, (long)chans);
    ALsetwidth(config, (long)dsize);
    params[0] = AL_OUTPUT_RATE;
    params[1] = (long)srate;
    ALsetparams(AL_DEFAULT_DEVICE, params, 2*NUM_PARAMS);
    playPort = ALopenport("SGIplay", "w", config);
    ALfreeconfig(config);
}

void play_on(short *buf, long csize)    /* the number of SAMPLE FRAMES */
{
    ALwritesamps(playPort, buf, csize*nchans);
}

void play_rls(void)
{
    while(ALgetfilled(playPort)>0)
      sginap(1);
    ALcloseport(playPort);
}

void play(      /* called from main place */
    short   *stt,
    short   *end,
    int     chans,
    int     dsize,      /* ignored - assumed = sizeof(short) in what follows */
    MYFLT   srate,
    int     scale       /* crude gain - ignored here */
  )
{
    short       *src;
    int         chunkSamps;
    int         bestSize;

    play_set(chans,dsize,srate,scale);

    src = stt;
    bestSize = 1024*chans; /* (hbBlck/2)*chans; /* hblck/2 is frames per blk */
    while(src < end)            /* samples still to send */
        {
        chunkSamps = end-src;
        if (chunkSamps>bestSize) chunkSamps=bestSize;
        play_on(src,chunkSamps);
        src += (chunkSamps);    /* cs/2 is num of frames */
        }
    play_rls();         /* release for parallel code */
}

void rec_set(int chans, int dsize, MYFLT srate, int scale)
{
    ALconfig config;
    long params[2*NUM_PARAMS];

    rcdszstat = dsize;  /* remember what it is set to */
    config = ALnewconfig();
    ALsetchannels(config, (long)chans);
    ALsetwidth(config, (long)dsize);
    params[0] = AL_INPUT_RATE;
    params[1] = (long)srate;
    ALsetparams(AL_DEFAULT_DEVICE, params, 2*NUM_PARAMS);
    recPort = ALopenport("SGIrec", "r", config);
    ALfreeconfig(config);
}

void rec_on(short *buf, long csize)     /* the number of SAMPLES */
{
    ALreadsamps(recPort, buf, csize);
}

void rec_rls(void)
{
/*    while(ALgetfilled(recPort)>0)
      sginap(1);    */
    ALcloseport(recPort);
}

void record(    /* called from main place */
    short   *stt,
    short   *end,
    int     chans,
    int     dsize,      /* ignored - assumed = sizeof(short) in what follows */
    MYFLT   srate,
    int     scale)      /* crude gain - ignored here */
{
    short       *src;
    int         chunkSamps;
    int         bestSize;

    rec_set(chans,dsize,srate,scale);

    src = stt;
    bestSize = 1024*chans; /* (hbBlck/2)*chans; /* hblck/2 is frames per blk */
    while(src < end)            /* samples still to send */
        {
        chunkSamps = end-src;
        if (chunkSamps>bestSize) chunkSamps=bestSize;
        rec_on(src,chunkSamps);
        src += (chunkSamps);    /* cs/2 is num of frames */
        }
    rec_rls();          /* release for parallel code */
}

