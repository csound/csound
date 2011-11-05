/*
    Copyright (C) 2007 Simon Schampijer

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

#include <vorbis/vorbisfile.h>
#include <inttypes.h>

#include "csdl.h"

#define OGGREAD_MAXCHAN 2
#define OGGREAD_BUFLEN 1024

typedef struct
{
    OPDS h;
    MYFLT *aout[OGGREAD_MAXCHAN]; /* outarg */
    MYFLT* *ifilename, *iseek;    /* inargs */
    OggVorbis_File vf;
    int bs;
    int nsamples;
    int doperf;
    int nchannels;
    int16_t *pint;
    void *pbuf[OGGREAD_BUFLEN];
} OGGREAD;


int oggread_init (CSOUND *csound, OGGREAD * p)
{
    FILE *in;
    char name[1024];
    float iseek = *p->iseek;
    vorbis_info *vi;

    /* check number of channels */
    p->nchannels = (int) (p->OUTOCOUNT);
    if (p->nchannels < 1 || p->nchannels > OGGREAD_MAXCHAN) {
      return csound->InitError(csound, Str("oggread: invalid number of channels"));
    }

    csound->strarg2name(csound, name, p->ifilename, "oggread.", p->XSTRCODE);
    in = fopen(name, "rb");
    if (!in) {
      return csound->InitError(csound, Str("oggread: Failed to open file"));
    }

    if (ov_open(in, &p->vf, NULL, 0) < 0) {
      fclose(in);
      return csound->InitError(csound,
                               Str("oggread: Failed to open input as vorbis"));
    }

    /* check number of channels in file (must be equal the number of outargs) */
    if ((vi=ov_info(&p->vf, 0))->channels != p->nchannels) {
      return csound->InitError(csound, Str("oggread: number of output args "
                                           "inconsistent with number of file "
                                           "channels"));
    }

    /* Check samplerate */
    if ((MYFLT)(vi->rate) != csound->esr) {
      csound->Warning(csound, Str("oggread: sample rate of file is %d "
                                  "which coes not match sr (=%f)\n"),
                                  vi->rate, csound->esr);
    }
    p->bs = 0;
    p->nsamples = 0;
/*     p->pint=NULL; */
    p->doperf = 1;

    if (iseek) {
      if (ov_seekable(&p->vf)) {
        /* get the total time in seconds of the physical bitstream */
        double length=ov_time_total(&p->vf,-1);
        if (length > iseek) {
          csound->Warning(csound, Str("oggread: seek file to sec=%f\n"), iseek);
          ov_time_seek(&p->vf, iseek);
        }
        else
          csound->Warning(csound,
                          Str("oggread: seek_point=%f > file_length=%f\n"),
                          iseek, length);
      }
      else
        csound->Warning(csound, Str("oggread: file is not seekable\n"));
    }
    return OK;
}

int oggread_perf (CSOUND *csound, OGGREAD * p)
{
    int ret;
    int i, nsmps=csound->ksmps;

    for (i = 0; i < nsmps; i++) {
      if (p->doperf == 1) {
        if (p->nsamples < p->nchannels) {
          if ((ret = ov_read(&p->vf, p->pbuf, OGGREAD_BUFLEN,
                             0, 2, 1, &p->bs)) != 0) {
            if (p->bs != 0)
              csound->Warning(csound,
                              Str("oggread: Only one logical bitstream "
                                  "currently supported\n"));
            if (ret < 0 )
              csound->Warning(csound, Str("oggread: Warning hole in data\n"));
            p->pint = (int16_t*) p->pbuf;
            p->nsamples = ret/2;
            p->doperf = 1;
          }
          else {
            ov_clear(&p->vf);
            /* End of file */
            p->doperf = 0;
            return OK;
          }
        }
        if (p->nchannels == 1) {
          p->aout[0][i] = (MYFLT) (*p->pint++);
        }
        else if (p->nchannels == 2) {
          p->aout[0][i] = (MYFLT) (*p->pint++);
          p->nsamples--;
          p->aout[1][i] = (MYFLT) (*p->pint++);
        }
        p->nsamples--;
      }
      else {
        if (p->nchannels == 1)
          p->aout[0][i] = FL(0.0);
        else if (p->nchannels == 2) {
          p->aout[0][i] = FL(0.0);
          p->aout[1][i] = FL(0.0);
        }
      }
    }
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY ogg_localops[] = {
  { "oggread",  S(OGGREAD),  5, "mm", "To",
    (SUBR) oggread_init, (SUBR) NULL, (SUBR) oggread_perf }
};

LINKAGE1(ogg_localops)
