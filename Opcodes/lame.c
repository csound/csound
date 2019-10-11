/*
  mp3in.c:

  Copyright (C) 2019 by John ffitch

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

#include "csdl.h"
#include "lame.h"

typedef struct _mp3out {
  OPDS              h;
  MYFLT             *al;
  MYFLT             *ar;
  STRINGDAT         *filename;
  lame_global_flags *gfp;
  FILE              *fout;
  AUXCH             auxch;
  void              *mp3buffer;
  int               mp3buffer_size;

  MYFLT             *leftpcm;
  MYFLT             *rightpcm;
} MP3OUT;

#ifdef USE_DOUBLE
#define lame_encode_buffer_ieee_MYFLT lame_encode_buffer_ieee_double
#else
#define lame_encode_buffer_ieee_MYFLT lame_encode_buffer_ieee_float
#endif

int mp3out_cleanup(CSOUND *csound, MP3OUT *p)
{
    IGN(csound);
    int bytes = lame_encode_flush(p->gfp,p->mp3buffer, p->mp3buffer_size);
    if (bytes>0) fwrite(p->mp3buffer, 1, bytes, p->fout);
    lame_mp3_tags_fid(p->gfp, p->fout);
    lame_close(p->gfp);

    p->gfp = NULL;
    fclose(p->fout);
    return OK;
}


int mp3out_init(CSOUND *csound, MP3OUT* p)
{
    int ret_code;
    unsigned int nsmps = csound->GetKsmps(csound);
    lame_global_flags *gfp;
    gfp = lame_init();
    p->gfp = gfp;
    
    lame_set_num_channels(gfp,2);
    lame_set_in_samplerate(gfp,csound->GetSr(csound));
    lame_set_brate(gfp,128);
    lame_set_mode(gfp,1);
    lame_set_quality(gfp,2);   /* 2=high  5 = medium  7=low */ 
    if (UNLIKELY(ret_code = lame_init_params(gfp)) <0)
      return csound->InitError(csound,
                               Str("Failed to initialise LAME %d\n"), ret_code);
    p->fout = fopen(p->filename->data, "w+b");
    if (p->fout ==NULL)
      return
        csound->InitError(csound, Str("mp3out %s: failed to open file"),
                          p->filename->data);
    csound->AuxAlloc(csound,
                     2*nsmps*sizeof(MYFLT)+(p->mp3buffer_size=3*nsmps/2+7200),
                     &p->auxch);
    p->mp3buffer = p->auxch.auxp;
    p->leftpcm = p->auxch.auxp+p->mp3buffer_size;
    p->rightpcm = p->leftpcm + sizeof(MYFLT)*nsmps;
    csound->RegisterDeinitCallback(csound, p,
                                   (int32_t (*)(CSOUND*, void*)) mp3out_cleanup);
    return OK;
}

int mp3out_perf(CSOUND *csound, MP3OUT *p)
{
    int bytes;
    unsigned int i, nsmps = csound->GetKsmps(csound);
    MYFLT zdbfs = csound->Get0dBFS(csound);
    for (i = 0; i<nsmps; i++) { /* Normalise if necessary */
      p->leftpcm[i] =  p->al[i]/zdbfs;
      p->rightpcm[i] = p->ar[i]/zdbfs;
    }
    bytes = lame_encode_buffer_ieee_MYFLT(p->gfp,
         p->leftpcm, p->rightpcm,
         nsmps, p->mp3buffer,  p->mp3buffer_size);
    if (bytes>=0) fwrite(p->mp3buffer, 1, bytes, p->fout);
    else if (bytes<0)
      return csound->PerfError(csound, &(p->h),
                               Str("mp3out: write error %d\n"), bytes);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "mp3out", S(MP3OUT), 0, 3, "", "aaS", (SUBR)mp3out_init, (SUBR)mp3out_perf}
};

LINKAGE
