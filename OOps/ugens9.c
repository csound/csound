/*  
    ugens9.c:

    Copyright (C) 1996 Greg Sullivan

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

#include "cs.h"         /*                                      UGENS9.C        */
#include <math.h>
#include "fft.h"
#include "dsputil.h"
#include "convolve.h"
#include "ugens9.h"
#include "soundio.h"
#include "oload.h"

/* static  int     pdebug = 0; */
/* static  int     dchan = 6; */      /* which channel to examine on debug */

int cvset(CONVOLVE *p)
{
    char     cvfilnam[MAXNAME];
    MEMFIL   *mfp, *ldmemfile(char *);
    CVSTRUCT *cvh;
    long     Hlenpadded = 1, obufsiz, Hlen;
    int      nchanls;

    if (O.odebug) printf(CONVOLVE_VERSION_STRING);

    if (*p->ifilno == SSTRCOD) {                         /* if strg name given */
      extern char *unquote(char *name);
      if (p->STRARG == NULL) strcpy(cvfilnam,unquote(currevent->strarg));
      else strcpy(cvfilnam, unquote(p->STRARG));
    }
    else if ((long)*p->ifilno <= strsmax && strsets != NULL &&
             strsets[(long)*p->ifilno])
      strcpy(cvfilnam, strsets[(long)*p->ifilno]);
    else sprintf(cvfilnam,
                 "convolve.%d", (int)*p->ifilno); /* else convolve.filnum   */
    if ((mfp = p->mfp) == NULL || strcmp(mfp->filename, cvfilnam) != 0)
                                /* if file not already readin */
      if ( (mfp = ldmemfile(cvfilnam)) == NULL) {
        sprintf(errmsg,Str(X_196,"CONVOLVE cannot load %s"), cvfilnam);
        goto cverr;
      }
    cvh = (CVSTRUCT *)mfp->beginp;
    if (cvh->magic != CVMAGIC) {
      sprintf(errmsg,Str(X_59,"%s not a CONVOLVE file (magic %ld)"),
              cvfilnam, cvh->magic );
      goto cverr;
    }

    nchanls = (cvh->channel == ALLCHNLS ? cvh->src_chnls : 1);

    if (*p->channel == 0.) {
      if (p->OUTOCOUNT == nchanls)
        p->nchanls = nchanls;
      else {
        sprintf(errmsg,
                Str(X_198,
                    "CONVOLVE: output channels not equal "
                    "to number of channels in source"));
        goto cverr;
      }
    }
    else {
      if (*p->channel <= nchanls) {
        if (p->OUTOCOUNT != 1) {
          sprintf(errmsg,
                  Str(X_198,"CONVOLVE: output channels not equal "
                      "to number of channels in source"));
          goto cverr;
        }
        else
          p->nchanls = 1;
      }
      else {
        sprintf(errmsg,
                Str(X_197,"CONVOLVE: channel number greater than "
                    "number of channels in source"));
        goto cverr;
      }
    }
    Hlen = p->Hlen = cvh->Hlen;
    while (Hlenpadded < 2*Hlen-1)
      Hlenpadded <<= 1;
    p->Hlenpadded = Hlenpadded;
    p->H = (MYFLT *) ((char *)cvh+cvh->headBsize);
    if ((p->nchanls == 1) && (*p->channel > 0))
      p->H += (Hlenpadded + 2) * (int)(*p->channel - 1);

    if ((cvh->samplingRate) != esr &&
        (O.msglevel & WARNMSG)) { /* & chk the data */
      printf(Str(X_63,"WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
              cvfilnam, cvh->samplingRate, esr);
    }
    if (cvh->dataFormat != CVMYFLT) {
      sprintf(errmsg,Str(X_1357,"unsupported CONVOLVE data format %ld in %s"),
              cvh->dataFormat, cvfilnam);
      goto cverr;
    }

   /* Initialise FFT lookup table */
    p->cvlut = (MYFLT *)AssignBasis(NULL, Hlenpadded);

/* Determine size of circular output buffer */
    if (Hlen >= ksmps)
      obufsiz = (long)ceil( (double)Hlen / (double)ksmps ) * ksmps;
    else
      obufsiz = (long)ceil( (double)ksmps / (double)Hlen ) * Hlen;

    if (p->auxch.auxp == NULL) {              /* if no buffers yet, alloc now */
      MYFLT *fltp;
      auxalloc((long)(((Hlenpadded+2) + p->nchanls*((Hlen - 1) + obufsiz)
                       + (p->nchanls > 1 ? (Hlenpadded+2) : 0))*sizeof(MYFLT)),
               &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->fftbuf = fltp;   fltp += (Hlenpadded + 2); /* and insert addresses */
      p->olap = fltp;     fltp += p->nchanls*(Hlen - 1);
      p->outbuf = fltp;   fltp += p->nchanls*obufsiz;
      p->X  = fltp;
    }
    p->obufsiz = obufsiz;
    p->outcnt = obufsiz;
    p->incount = 0;
    p->obufend = p->outbuf + obufsiz - 1;
    p->outhead = p->outail = p->outbuf;
    return OK;
 cverr:
    return initerror(errmsg);
}

extern void writeFromCircBuf(MYFLT **, MYFLT **, MYFLT *, MYFLT *, long);

int convolve(CONVOLVE *p)
{
    int    nsmpso=ksmps,nsmpsi=ksmps,nsmpso_sav,outcnt_sav;
    int    nchm1 = p->nchanls - 1,chn;
    long   i,j;
    MYFLT  *ar[4];
    MYFLT  *ai = p->ain;
    MYFLT  *fftbufind;
    long   outcnt = p->outcnt;
    long   incount=p->incount;
    long   Hlen = p->Hlen;
    long   Hlenm1 = Hlen - 1;
    long   obufsiz = p->obufsiz;
    MYFLT  *outhead;
    MYFLT  *outail = p->outail;
    MYFLT  *olap;
    MYFLT  *X;
    long   Hlenpadded = p->Hlenpadded;

    ar[0] = p->ar1;
    ar[1] = p->ar2;
    ar[2] = p->ar3;
    ar[3] = p->ar4;

    if (p->auxch.auxp==NULL) {
      return perferror(Str(X_667,"convolve: not initialised"));
    }
  /* First dump as much pre-existing audio in output buffer as possible */
    if (outcnt > 0) {
      if (outcnt <= ksmps)
        i = outcnt;
      else
        i = ksmps;
      nsmpso -= i; outcnt -= i;
      for (chn = nchm1;chn >= 0;chn--) {
        outhead = p->outhead + chn*obufsiz;
        writeFromCircBuf(&outhead,&ar[chn],p->outbuf+chn*obufsiz,
                         p->obufend+chn*obufsiz,i);
      }
      p->outhead = outhead;
    }
    while (nsmpsi > 0) {
/* Read input audio and place into work buffer. */

      fftbufind = p->fftbuf + incount;
      if ((incount + nsmpsi) <= Hlen)
        i = nsmpsi;
      else
        i = Hlen - incount;
      nsmpsi -= i;
      incount += i;
      while (i--)
        *fftbufind++ = *ai++;
      if (incount == Hlen) {
        /* We have enough audio for a convolution. */
        incount = 0;
        /* FFT the input (to create X) */
        /*printf("CONVOLVE: ABOUT TO FFT\n"); */
        FFT2realpacked((complex *)p->fftbuf,Hlenpadded,
                       (complex *)(p->cvlut));
        /* save the result if multi-channel */
        if (nchm1) {
          fftbufind = p->fftbuf;
          X = p->X;
          for (i = Hlenpadded + 2;i > 0;i--)
            *X++ = *fftbufind++;
        }
        nsmpso_sav = nsmpso;
        outcnt_sav = outcnt;
        for (chn = nchm1;chn >= 0;chn--) {
          outhead = p->outhead + chn*obufsiz;
          outail = p->outail + chn*obufsiz;
          olap = p->olap + chn*Hlenm1;
          if (chn < nchm1) {
            fftbufind = p->fftbuf;
            X = p->X;
            for (i = Hlenpadded + 2;i> 0;i--)
              *fftbufind++ = *X++;
          }
          /*printf("CONVOLVE: ABOUT TO MULTIPLY\n");  */
          /* Multiply H * X, point for point */
          cxmult((complex *)(p->H+chn*(Hlenpadded+2)),
                 (complex *)(p->fftbuf),Hlenpadded/2 + 1);

          /*printf("CONVOLVE: ABOUT TO IFFT\n"); */
          /* Perform inverse FFT on X */

          FFT2torlpacked((complex*)(p->fftbuf),Hlenpadded,
                         FL(1.0)/Hlenpadded,(complex*)(p->cvlut));

          /* Take the first Hlen output samples and output them to
             either the real audio output buffer or the local circular
             buffer */
          nsmpso = nsmpso_sav;
          outcnt = outcnt_sav;
          fftbufind = p->fftbuf;
          if ( (nsmpso > 0)&&(outcnt == 0) ) {
            /*    printf("Outputting to audio buffer proper\n");*/
            /* space left in output buffer, and nothing currently in circular
               buffer, so write as much as possible to output buffer first */
            if (nsmpso >= Hlenm1) {
              nsmpso -= Hlenm1;
              for (i=Hlenm1;i > 0;--i)
                *ar[chn]++ = *fftbufind++ + *olap++;
              if (nsmpso > 0) {
                *ar[chn]++ = *fftbufind++;
                --nsmpso;
              }
            }
            else {
              for (;nsmpso > 0;--nsmpso)
                *ar[chn]++ = *fftbufind++ + *olap++;
            }
          }
/* Any remaining output must go into circular buffer */
/*printf("Outputting to circ. buffer\n");*/
          i = Hlen - (fftbufind - p->fftbuf);
          outcnt += i;
          i--; /* do first Hlen -1 samples with overlap */
          j = p->obufend+chn*obufsiz - outail + 1;
          if (i >= j) {
            i -= j;
            for (;j > 0;--j)
              *outail++ = *fftbufind++ + *olap++;
            outail = p->outbuf+chn*obufsiz;
          }
          for (;i > 0;--i)
            *outail++ = *fftbufind++ + *olap++;
/*  just need to do sample at Hlen now */
          *outail++ = *fftbufind++;
          if (outail > p->obufend+chn*obufsiz)
            outail = p->outbuf+chn*obufsiz;

/*  Pad the rest to zero, and save first remaining (Hlen - 1) to overlap
    buffer */
          olap = p->olap+chn*Hlenm1;
          for (i = Hlenm1;i > 0;--i) {
            *olap++ = *fftbufind;
            *fftbufind++ = FL(0.0);
          }
          olap = p->olap+chn*Hlenm1;
    /* Now pad the rest to zero as well. In theory, this shouldn't be
       necessary, however it's conceivable that rounding errors may
       creep in, and these cells won't be exactly zero. So, let's
       make absolutely sure */
          for (i = Hlenpadded - (Hlen+Hlenm1);i > 0;--i)
            *fftbufind++ = FL(0.0);
        } /* end main for loop */
        p->outhead = outhead;
        p->outail = outail;
      }
    } /* end while */

/* update state in p */
    p->incount = incount;
    p->outcnt = outcnt;
    p->outhead = outhead;
    p->outail = outail;
    return OK;
}
