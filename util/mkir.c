/*
    mkir.c

    Copyright (C) 2026 V Lazzarini

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

/** 
   Utility for creating empirical impulse responses
   sweep generation: mkir -g sweep.wav -t dur -r sr 
   deconvolution: mkir sweep.wav -i infile -o outfile
*/

#include "std_util.h"
#include "arrays.h"

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
      csound->Die(csound, "%s", MSG);

static MYFLT *generate_sweep(CSOUND *csound, MYFLT sr, int32_t len) {
  int n;
  double ph = 0, f = 1., fi;
  double scal = 2*PI/sr;
  MYFLT *sweep = (MYFLT *) csound->Calloc(csound, len*sizeof(MYFLT));
  fi = pow(sr/2, 1./len);
  for(n = 0; n < len; n++) {
    sweep[n] = sin(ph);
    ph += scal*f;
    f *= fi;
  }
  return sweep;
}

static void specdiv(CSOUND *csound, MYFLT *inp, MYFLT *swp, int32_t fftlen) {
   void *setup = csound->RealFFTSetup(csound, fftlen, FFT_FWD);
   csound->RealFFT(csound, setup, inp);
   csound->RealFFT(csound, setup, swp);
   if(swp[0] != 0)
     inp[0] /= swp[0];
   if(swp[1] != 0)
     inp[1] /= swp[1];
   for(int n = 2; n < fftlen; n+=2) {
     MYFLT c = swp[n], a = inp[n];
     MYFLT d = swp[n+1], b = inp[n+1];
     MYFLT den = c*c + d*d;
     if(den == 0)
       csound->Warning(csound, "deconv: div by zero detected, sweep bin %d", n/2);
     else {
      inp[n] = (a*c + b*d)/den;
      inp[n+1] = (b*c - a*d)/den;
     }
   }
   setup = csound->RealFFTSetup(csound, fftlen, FFT_INV);
   csound->RealFFT(csound, setup, inp);
}

  
static MYFLT *deconvolve(CSOUND *csound, MYFLT *sweep, MYFLT *rec, int32_t len,
                         int32_t ilen) {
   int32_t fftlen = 2*len;
   MYFLT *inp = (MYFLT *) csound->Calloc(csound,fftlen*sizeof(MYFLT));
   MYFLT *swp = (MYFLT *) csound->Calloc(csound,fftlen*sizeof(MYFLT));
   memcpy(inp, rec, sizeof(MYFLT)*(ilen < fftlen ? ilen : fftlen));
   memcpy(swp, sweep, sizeof(MYFLT)*len);
   specdiv(csound,inp,swp,fftlen);
   memcpy(rec,inp,sizeof(MYFLT)*len);
   csound->Free(csound,inp);
   csound->Free(csound,swp);
   return rec;
}


static void usage(CSOUND *csound, char *mesg, ...)
{
    va_list args;
    csound->Message(csound,"%s", Str("Usage:\tmkir [-flags] sweepfile \n"));
    csound->Message(csound,"%s", Str("Legal flags are:\n"));
    csound->Message(csound,"%s", Str("-g generate sine sweep (RIFF-Wave, float)\n"));
    csound->Message(csound,"%s", Str("-t sweep length (in secs) to generate\n"));
    csound->Message(csound,"%s", Str("-r sampling rate for sweep generation\n"));
    csound->Message(csound,"%s", Str("-o output file\n"));
    csound->Message(csound,"%s", Str("-i input file\n"));
    csound->Message(csound,"%s", Str("flag defaults: mkir -t 1 -r 44100 \n"));
    va_start(args, mesg);
    csound->ErrMsgV(csound, Str("mkdir: error: "), mesg, args);
    va_end(args);
    csound->LongJmp(csound, 1);
}

static int32_t mkir(CSOUND *csound, int32_t argc, char **argv) {
  char *sweepfile = NULL, *inputfile = NULL, *outputfile = NULL;
  int32_t generate = 0;
  MYFLT len = 1., sr = 44100.;
  char *s, c;

  if (UNLIKELY(!(--argc))) {
      usage(csound,Str("Insufficient arguments"));
      return 1;
  }
  do {
      s = *++argv;
      if (*s++ == '-')                
        while ((c = *s++) != '\0')
          switch(c) {
          case 'o':
            FIND(Str("no output filename"))
            outputfile = s;         
            for ( ; *s != '\0'; s++) ;
            if (UNLIKELY(strcmp(outputfile, "stdin") == 0))
              csound->Die(csound, "%s", Str("-o cannot be stdin"));
            break;
          case 'i':
            FIND(Str("no input filename"))
            inputfile = s;        
            for ( ; *s != '\0'; s++) ;
            if (UNLIKELY(strcmp(inputfile, "stdout") == 0))
              csound->Die(csound, "%s", Str("-i cannot be stdout"));
            break;
          case 'g':
            generate = 1;         
            break;
          case 't':
            FIND(Str("no sweep length"));
            len = (MYFLT) atof(s);
            while (*++s);           
            break;
          case 'r':
            FIND(Str("no sampling rate"));
            sr = (MYFLT) atof(s);
            while (*++s);           
            break;            
          default:
            usage(csound, Str("unknown flag -%c"), c);            
          }
      else {
        if (UNLIKELY(sweepfile != NULL)) usage(csound,Str("Too many inputs"));
        sweepfile = --s;
      }
    } while (--argc);

  if(sweepfile == NULL)
    csound->Die(csound, "%s", Str("missing sweep file"));
  
  if(generate == 0) {
     SFLIB_INFO sfinfo;
     SNDFILE *fd;
     int32_t frames, iframes;
     MYFLT *swp, *inp;
     if (inputfile == NULL)
       csound->Die(csound, "%s", Str("missing input file"));
     if (outputfile == NULL)
       csound->Die(csound, "%s", Str("missing output file"));
     
     fd = csound->SndfileOpen(csound, sweepfile,
                                 SFM_READ, &sfinfo);
     if(fd == NULL)
       csound->Die(csound, "%s", Str("could not open sweep file"));
       
     if(sfinfo.channels > 1) {
        csound->SndfileClose(csound, fd);
        csound->Die(csound, "%s", Str("sweep file is not mono"));
     }
     frames = (int32_t) sfinfo.frames; 
     swp = (MYFLT *) csound->Calloc(csound, frames*sizeof(MYFLT));
     csound->SndfileRead(csound,fd,swp,frames);
     csound->SndfileClose(csound,fd);

     memset(&sfinfo, 0, sizeof(SFLIB_INFO));
     fd = csound->SndfileOpen(csound, inputfile, SFM_READ,
                              &sfinfo);
     if(fd == NULL)
       csound->Die(csound, "%s", Str("could not open input file"));
     
     iframes = (int32_t) (sfinfo.frames > frames ? sfinfo.frames : frames);
     inp = (MYFLT *) csound->Calloc(csound, iframes*sizeof(MYFLT)*sfinfo.channels);
     csound->SndfileRead(csound,fd,inp,iframes);
     csound->SndfileClose(csound,fd);
     
     if(sfinfo.channels > 1) {
       int i, j, m = sfinfo.channels;
       MYFLT *outp, *chn = (MYFLT *)
         csound->Calloc(csound, len*sizeof(MYFLT));
       csound->Message(csound,
                     "\tmultichannel input: %d channels\n",
                     sfinfo.channels);       
       for(i = 0; i < sfinfo.channels; i++) {
         csound->Message(csound,
                     "\t\tprocessing channel %d\n",
                     i);    
         for(j = 0; j < iframes; j++)
             chn[j] = inp[j*m + i];
         outp = deconvolve(csound, swp, chn, frames, iframes);
         for(j = 0; j < iframes; j++)
             inp[j*m + i] = outp[j];
       }
       csound->Free(csound, chn);
     } else inp = deconvolve(csound, swp, inp, frames, iframes);
     sfinfo.frames = 0;
     fd = csound->SndfileOpen(csound, outputfile, SFM_WRITE, &sfinfo);

     if(fd == NULL)
       csound->Die(csound, "%s", Str("could not open output file"));
     
     csound->SndfileWrite(csound,fd,inp,frames);
     csound->SndfileClose(csound,fd);
     csound->Free(csound, swp);
     csound->Free(csound, inp);
     csound->Message(csound,
                     "\tcreated IR file %s\n",
                     outputfile);
     csound->Message(csound,
                     "\tsr = %.1f, %.3f seconds, %d channels\n",
                     sr, frames/sr, sfinfo.channels);
   }
  else {
    SFLIB_INFO sfinfo;
    SNDFILE* fd;
    MYFLT *sweep = generate_sweep(csound, sr, len*sr);
    memset(&sfinfo, 0, sizeof(SFLIB_INFO));
    sfinfo.samplerate = sr;
    sfinfo.channels = 1;
    sfinfo.format = TYPE2SF(TYP_WAV) | FORMAT2SF(AE_FLOAT);
    fd = csound->SndfileOpen(csound, sweepfile, SFM_WRITE, &sfinfo);
    if (UNLIKELY(fd == NULL))
      csound->Die(csound, Str("Failed to open file for generated sweep %s: %s"),
                 sweepfile, Str(csound->SndfileStrError(csound,NULL)));
    csound->SndfileWrite(csound,fd,sweep,len*sr);
    csound->SndfileClose(csound,fd);
    csound->Free(csound, sweep);
    csound->Message(csound,
                     "\tcreated sine sweep file %s\n",
                     sweepfile);
    csound->Message(csound,
                     "\tsr = %.1f, %.3f seconds\n",
                     sr, len);
  }
  return CSOUND_SUCCESS;
}

typedef struct {
  OPDS h;
  ARRAYDAT *outp, *inp, *swp;
  MYFLT *in, *sw;
  int32_t len;
} DECONV;

// deconv_init allocates memory for operations
static int32_t deconv_init(CSOUND *csound, DECONV *p) {
  int32_t fftlen;
  p->len = p->swp->sizes[0];
  fftlen = p->len*2;
  tabinit_like(csound, p->outp, p->swp);
  p->in = (MYFLT *) csound->Calloc(csound, sizeof(MYFLT)*fftlen);
  p->sw = (MYFLT *) csound->Calloc(csound, sizeof(MYFLT)*fftlen);
  return OK;
}

// frees allocated memory
static int32_t deconv_deinit(CSOUND *csound, DECONV *p) {
  csound->Free(csound, p->in);
  csound->Free(csound, p->sw);
  return OK;
}

// copies the data and applies spectral division
// then copies the data to output array 
static int32_t kdeconv(CSOUND *csound, DECONV *p) {
  int32_t fftlen = p->len*2;
  memcpy(p->sw, p->swp->data, sizeof(MYFLT)*p->len);
  memcpy(p->in, p->inp->data, sizeof(MYFLT)*(p->inp->sizes[0]
                                             < fftlen ?
                                             p->inp->sizes[0] :
                                             fftlen));
  specdiv(csound,p->in,p->sw,fftlen);
  memcpy(p->outp->data,p->in, sizeof(MYFLT)*p->len);
  return OK;
}

// all above operations in one go at i-time.
static int32_t ideconv(CSOUND *csound, DECONV *p) {
  deconv_init(csound, p);
  kdeconv(csound, p);
  deconv_deinit(csound, p);
  return OK;
}


static int32_t gen_deconv(FGDATA *ff, FUNC *ftp) {
  CSOUND  *csound = ff->csound;
  FUNC    *inp = csound->FTFind(csound, &(ff->e.p[6]));
  FUNC    *sweep = csound->FTFind(csound, &(ff->e.p[5]));
  int32_t len = sweep->flen;
  MYFLT   *fp;
  MYFLT   *inpd;
  int32_t chns = ff->e.pcnt - 5;

  if(ff->e.p[5] <= 0) {
    csound->Message(csound, "sweep table num %d illegal", (int) ff->e.p[5]);
    return NOTOK;
  }
                        
  
  if(chns < 1) {
    csound->Message(csound, "insufficient number of input channels: %d", chns);
    return NOTOK;
  }

  if(ftp) {
   fp = ftp->ftable;
   if(len != ftp->flen/chns) {
     csound->Message(csound, "destination table size not matching"
                     " sweep, size %d frames, need %d\n", ftp->flen/chns, len);
    return NOTOK;
   }
  }
  else {
    ff->e.p[3] = len*chns;
    csound->FTCreate(csound, &ftp, &ff->e, ff->e.p[1]);
    if(ftp)
    // table allocated, exit.
    return OK;
    else return NOTOK;
  }
  
  sweep = csound->FTFind(csound, &(ff->e.p[5]));
  for(int n = 0; n < chns; n++) {
    if(ff->e.p[n+6] <= 0) {
      csound->Message(csound, "input table num %d illegal", (int) ff->e.p[n+6]);
     return NOTOK;
    }
    inp = csound->FTFind(csound, &(ff->e.p[n+6]));
    
    // input length cannot be smaller than sweep length
    if(len) {
      int32_t ilen =  inp->flen > len ? inp->flen : len;
      inpd = (MYFLT *) csound->Calloc(csound, ilen*sizeof(MYFLT));  
      memcpy(inpd, inp->ftable, sizeof(MYFLT)*inp->flen);
      deconvolve(csound, sweep->ftable, inpd, len, ilen);
      for(int i = 0; i < len; i++)
        fp[i*chns + n] = inpd[i];  
      csound->Free(csound, inpd);
    } else {
      csound->Message(csound, "illegal sweep source table length: %d\n", len);
      return NOTOK;
    }
  }
  return OK;
}


int32_t mkir_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "mkir", mkir);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "mkir",
                                             Str("Creates empirical impulse responses"));
    }
    csound->AppendOpcode(csound, "deconv", sizeof(DECONV), 0, "k[]", "k[]k[]",
                         (SUBR) deconv_init, (SUBR) kdeconv, (SUBR) deconv_deinit);
    csound->AppendOpcode(csound, "deconv", sizeof(DECONV), 0, "i[]", "i[]i[]",
                         (SUBR) ideconv, NULL, NULL);
    return retval;
}

static NGFENS nfgen[] = {{"deconv", gen_deconv}, {NULL, NULL}};
NGFENS *mkir_fgen_init(CSOUND *csound) {
   (void) csound;
   return nfgen;
}

#ifdef BUILD_PLUGINS
PUBLIC NGFENS *csound_fgen_init(CSOUND *csound) {
  return mkir_fgen_init(csound);
}
#endif
