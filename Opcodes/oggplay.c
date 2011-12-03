
#include <vorbis/vorbisfile.h>
#include <inttypes.h>

#include "csdl.h"

#define OGGPLAY_MAXCHAN 2
#define OGGPLAY_BUFLEN 1024

typedef struct
{
    OPDS h;
    MYFLT *aout[OGGPLAY_MAXCHAN]; /* outarg */
    MYFLT* *ifilename, *iseek;  /* inargs */
    OggVorbis_File vf;
    int bs;
    int nsamples;
    int buflen;
    int doperf;
    int nchannels;
    int16_t *pint;
    AUXCH pbuf;
} OGGPLAY;


int oggplay_init (CSOUND *csound, OGGPLAY * p)
{
    FILE *in;
    char name[1024];
    float iseek = *p->iseek;

    /* check number of channels */
    p->nchannels = (int) (p->OUTOCOUNT);
    if (p->nchannels < 1 || p->nchannels > OGGPLAY_MAXCHAN) {
      return csound->InitError(csound, Str("oggplay: invalid number of channels"));
    }

    csound->strarg2name(csound, name, p->ifilename, "oggplay.", p->XSTRCODE);
    in = fopen(name, "rb");
    if (!in) {
      return csound->InitError(csound, Str("oggplay: Failed to open file"));
    }

    if (ov_open(in, &p->vf, NULL, 0) < 0) {
      fclose(in);
      return csound->InitError(csound,
                               Str("oggplay: Failed to open input as vorbis"));
    }

    /* check number of channels in file (must be equal the number of outargs) */
    if (ov_info(&p->vf, 0)->channels != p->nchannels) {
      return csound->InitError(csound, Str("oggplay: number of output args "
                                           "inconsistent with number of file"
                                           " channels"));
    }

    p->bs       = 0;
    p->nsamples = 0;
    p->buflen   = OGGPLAY_BUFLEN;
    p->pint     = NULL;
    p->doperf   = 1;

    csound->AuxAlloc(csound, p->buflen, &(p->pbuf));

    if (iseek) {
      if (ov_seekable(&p->vf)) {
        /* get the total time in seconds of the physical bitstream */
        double length=ov_time_total(&p->vf,-1);
        if (length > iseek){
          csound->Warning(csound, Str("oggplay: seek file to sec=%f \n"), iseek);
          ov_time_seek(&p->vf, iseek);
        }
        else
          csound->Warning(csound,
                          Str("oggplay: seek_point=%f > file_length=%f \n"),
                          iseek, length);
      }
      else
        csound->Warning(csound, Str("oggplay: file is not seekable \n"));
    }

    return OK;
}


int oggplay_perf (CSOUND *csound, OGGPLAY * p)
{
    int ret;
    int i, nsmps=csound->ksmps;


    for (i = 0; i < nsmps; i++) {
      if (p->doperf == 1) {
        if (p->nsamples < p->nchannels) {
          if ((ret = ov_read(&p->vf, p->pbuf.auxp,
                             p->buflen, 0, 2, 1, &p->bs)) != 0) {
            if (p->bs != 0)
              csound->Warning(csound,
                              Str("oggplay: Only one logical "
                                  "bitstream currently supported\n"));
            if (ret < 0 )
              csound->Warning(csound,
                              Str("oggplay: Warning hole in data\n"));
            p->pint = (int16_t*) p->pbuf.auxp;
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
          p->aout[0][i] = (MYFLT) (*p->pint);
        }
        else if (p->nchannels == 2) {
          p->aout[0][i] = (MYFLT) (*p->pint);
          p->pint += 1;
          p->nsamples -= 1;
          p->aout[1][i] = (MYFLT) (*p->pint);
        }
        p->pint += 1;
        p->nsamples -= 1;
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

static OENTRY oggplay_localops[] = {
    { "oggplay",  S(OGGPLAY),  5, "mm", "To",
      (SUBR) oggplay_init, (SUBR) NULL, (SUBR) oggplay_perf }
};

LINKAGE1(oggplay_localops)
