/*
    bformdec2.cpp

    Copyright (C) 2019 Pablo Zinemanas

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

#include <stdlib.h>
#include <math.h>
#ifdef BUILD_PLUGINS
 #include "csdl.h"
#else
 #include "csoundCore.h"
#endif
#include <string.h>
#include <new>


/* Band-splitting constants */
#define MAXZEROS 4
#define MAXPOLES 4

/* Ambisonics constants */
#define MAX_OUTPUTS 20
#define MAX_INPUTS 16

/* HRTF constants */
#define minelev (-40)
#define elevincrement (10)

/* max delay for min phase: a time value:
   multiply by sr to get no of samples for memory allocation */
#define maxdeltime (0.0011)

/* additional definitions for woodworth models */
//#define c (34400.0)

static const float nonlinitd[5] = {1.570024f, 1.378733f, 1.155164f, 1.101230f,1.0f};
static const float nonlinitd48k[5] =
  {1.549748f, 1.305457f, 1.124501f, 1.112852f,1.0f};
static const float nonlinitd96k[5] =
  {1.550297f, 1.305671f, 1.124456f, 1.112818f,1.0f};

/* number of measurements per elev: mit data const:read only, static:exists
   for whole process... */
static const int32_t elevationarray[14] =
  {56, 60, 72, 72, 72, 72, 72, 60, 56, 45, 36, 24, 12, 1 };

/* assumed mit hrtf data will be used here. Otherwise delay data would need
   to be extracted and replaced here... */
/*
static const float minphasedels[368] =
{
  0.000000f, 0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000204f,
  0.000249f, 0.000272f, 0.000295f, 0.000317f, 0.000363f, 0.000385f,
  0.000272f, 0.000408f, 0.000454f, 0.000454f, 0.000408f, 0.000385f,
  0.000363f, 0.000317f, 0.000295f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000181f, 0.000227f, 0.000249f,
  0.000272f, 0.000317f, 0.000363f, 0.000385f, 0.000454f, 0.000476f,
  0.000454f, 0.000522f, 0.000499f, 0.000499f, 0.000476f, 0.000454f,
  0.000408f, 0.000408f, 0.000385f, 0.000340f, 0.000295f, 0.000272f,
  0.000227f, 0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000091f, 0.000113f, 0.000159f, 0.000204f,
  0.000227f, 0.000272f, 0.000317f, 0.000317f, 0.000363f, 0.000408f,
  0.000363f, 0.000522f, 0.000476f, 0.000499f, 0.000590f, 0.000567f,
  0.000567f, 0.000544f, 0.000522f, 0.000499f, 0.000476f, 0.000454f,
  0.000431f, 0.000408f, 0.000385f, 0.000363f, 0.000317f, 0.000295f,
  0.000249f, 0.000204f, 0.000181f, 0.000136f, 0.000091f, 0.000045f,
  0.000000f, 0.000000f, 0.000045f, 0.000091f, 0.000113f, 0.000159f,
  0.000204f, 0.000249f, 0.000295f, 0.000317f, 0.000363f, 0.000340f,
  0.000385f, 0.000431f, 0.000476f, 0.000522f, 0.000544f, 0.000612f,
  0.000658f, 0.000658f, 0.000635f, 0.000658f, 0.000522f, 0.000499f,
  0.000476f, 0.000454f, 0.000408f, 0.000385f, 0.000363f, 0.000340f,
  0.000295f, 0.000272f, 0.000227f, 0.000181f, 0.000136f, 0.000091f,
  0.000045f, 0.000000f, 0.000000f, 0.000045f, 0.000091f, 0.000136f,
  0.000159f, 0.000204f, 0.000249f, 0.000295f, 0.000340f, 0.000385f,
  0.000431f, 0.000476f, 0.000522f, 0.000567f, 0.000522f, 0.000567f,
  0.000567f, 0.000635f, 0.000703f, 0.000748f, 0.000748f, 0.000726f,
  0.000703f, 0.000658f, 0.000454f, 0.000431f, 0.000385f, 0.000363f,
  0.000317f, 0.000295f, 0.000272f, 0.000227f, 0.000181f, 0.000136f,
  0.000091f, 0.000045f, 0.000000f, 0.000000f, 0.000045f, 0.000091f,
  0.000113f, 0.000159f, 0.000204f, 0.000249f, 0.000295f, 0.000340f,
  0.000385f, 0.000408f, 0.000454f, 0.000499f, 0.000544f, 0.000522f,
  0.000590f, 0.000590f, 0.000635f, 0.000658f, 0.000680f, 0.000658f,
  0.000544f, 0.000590f, 0.000567f, 0.000454f, 0.000431f, 0.000385f,
  0.000363f, 0.000317f, 0.000272f, 0.000272f, 0.000227f, 0.000181f,
  0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f, 0.000045f,
  0.000068f, 0.000113f, 0.000159f, 0.000204f, 0.000227f, 0.000272f,
  0.000317f, 0.000340f, 0.000385f, 0.000431f, 0.000454f, 0.000499f,
  0.000499f, 0.000544f, 0.000567f, 0.000590f, 0.000590f, 0.000590f,
  0.000590f, 0.000567f, 0.000567f, 0.000476f, 0.000454f, 0.000408f,
  0.000385f, 0.000340f, 0.000340f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000113f, 0.000159f, 0.000204f, 0.000249f,
  0.000295f, 0.000340f, 0.000363f, 0.000385f, 0.000431f, 0.000454f,
  0.000499f, 0.000522f, 0.000522f, 0.000522f, 0.000499f, 0.000476f,
  0.000454f, 0.000431f, 0.000385f, 0.000340f, 0.000317f, 0.000272f,
  0.000227f, 0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000204f,
  0.000227f, 0.000249f, 0.000295f, 0.000340f, 0.000363f, 0.000385f,
  0.000408f, 0.000431f, 0.000431f, 0.000431f, 0.000431f, 0.000408f,
  0.000385f, 0.000363f, 0.000317f, 0.000317f, 0.000272f, 0.000227f,
  0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000181f, 0.000204f, 0.000227f,
  0.000272f, 0.000295f, 0.000317f, 0.000340f, 0.000340f, 0.000363f,
  0.000363f, 0.000340f, 0.000317f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000113f, 0.000068f, 0.000023f, 0.000000f, 0.000045f,
  0.000068f, 0.000113f, 0.000159f, 0.000181f, 0.000204f, 0.000227f,
  0.000249f, 0.000249f, 0.000249f, 0.000227f, 0.000227f, 0.000181f,
  0.000159f, 0.000113f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000181f, 0.000181f,
  0.000181f, 0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000068f, 0.000091f, 0.000068f, 0.000045f,
  0.000000f, 0.000000f
};
*/

#ifdef WORDS_BIGENDIAN
static int32_t swap4bytes(CSOUND* csound, MEMFIL* mfp)
{
    char c1, c2, c3, c4;
    char *p = mfp->beginp;
    int32_t  size = mfp->length;

    while (size >= 4)
      {
        c1 = p[0]; c2 = p[1]; c3 = p[2]; c4 = p[3];
        p[0] = c4; p[1] = c3; p[2] = c2; p[3] = c1;
        size -= 4; p +=4;
      }

    return OK;
}
#else
static int32_t (*swap4bytes)(CSOUND*, MEMFIL*) = NULL;
#endif

/* HRTF classes (adapted from csound/Opcodes/hrtfopcodes.c) */
class hrtf {
    public:
  hrtf() {}
  virtual ~hrtf() {}
  virtual void init(void) = 0;
  virtual int32_t hrtfstat_init(CSOUND *csound, MYFLT elev, MYFLT angle, MYFLT radius, STRINGDAT *filel, STRINGDAT *filer, MYFLT srxl) = 0;
  virtual int32_t hrtfstat_process(CSOUND *csound, MYFLT *in, MYFLT *outsigl, MYFLT *outsigr, uint32_t offset, uint32_t early, uint32_t nsmps) = 0;
};

class hrtf_c : public hrtf {
private:
  /*    MYFLT *outsigl, *outsigr;
  //MYFLT *in, *iangle, *ielev;
  MYFLT *iangle, *ielev;
  STRINGDAT *ifilel, *ifiler;
  MYFLT *oradius, *osr;*/

  //STRINGDAT *ifilel_p, *ifiler_p;

  /*see definitions in INIT*/
  int32_t irlength_p, irlengthpad_p, overlapsize_p;
  MYFLT sroverN_p;

  int32_t counter_p;
  MYFLT sr_p;

  /* hrtf data padded */
  AUXCH hrtflpad_p,hrtfrpad_p;
  /* in and output buffers */
  AUXCH insig_p, outl_p, outr_p;

  /* memory local to perform method */
  /* insig fft */
  AUXCH complexinsig_p;
  /* hrtf buffers (rectangular complex form) */
  AUXCH hrtflfloat_p, hrtfrfloat_p;
  /* spectral data */
  AUXCH outspecl_p, outspecr_p;

  /* overlap data */
  AUXCH overlapl_p, overlapr_p;

  /* interpolation buffers */
  AUXCH lowl1_p, lowr1_p, lowl2_p, lowr2_p, highl1_p, highr1_p, highl2_p, highr2_p;

  /* buffers for impulse shift */
  AUXCH leftshiftbuffer_p, rightshiftbuffer_p;

  void *setup, *setup_pad, *isetup, *isetup_pad;

public:

  virtual void init(void) {

  }

  /* HRTF functions (adapted from csound/Opcodes/hrtfopcodes.c) */
  virtual int32_t hrtfstat_init(CSOUND *csound,
                                MYFLT elev, MYFLT angle, MYFLT r, STRINGDAT *ifilel, STRINGDAT *ifiler,
                                MYFLT sr)
  {
      /* left and right data files: spectral mag, phase format. */

      MEMFIL *fpl = NULL, *fpr = NULL;

      /* interpolation values */
      MYFLT *lowl1;
      MYFLT *lowr1;
      MYFLT *lowl2;
      MYFLT *lowr2;
      MYFLT *highl1;
      MYFLT *highr1;
      MYFLT *highl2;
      MYFLT *highr2;

      MYFLT *hrtflfloat;
      MYFLT *hrtfrfloat;

      MYFLT *hrtflpad;
      MYFLT *hrtfrpad;

      /*  MYFLT elev = *p->ielev;
          MYFLT angle = *p->iangle;
          MYFLT r = *p->oradius;
          MYFLT sr = *p->osr;*/

      /* pointers into HRTF files */
      float *fpindexl=NULL;
      float *fpindexr=NULL;

      /* time domain impulse length, padded, overlap add */
      int32_t irlength=0, irlengthpad=0, overlapsize=0;

      int32_t i, skip = 0;

      /* local interpolation values */
      MYFLT elevindexhighper, angleindex2per, angleindex4per;
      int32_t elevindexlow, elevindexhigh, angleindex1, angleindex2,
        angleindex3, angleindex4;
      MYFLT magl, magr, phasel, phaser, magllow, magrlow, maglhigh, magrhigh;

      /* local variables, mainly used for simplification */
      MYFLT elevindexstore;
      MYFLT angleindexlowstore;
      MYFLT angleindexhighstore;

      /* woodworth values */
      MYFLT radianangle, radianelev, itd=0, itdww, freq;

      /* shift */
      int32_t shift;
      MYFLT *leftshiftbuffer;
      MYFLT *rightshiftbuffer;

      /* sr */

      sr_p = sr;

      //char filel[MAXNAME] = "hrtf-44100-left.dat"; //../hrtf/
      //char filer[MAXNAME] = "hrtf-44100-right.dat";

      if (sr_p != FL(44100.0) && sr_p != FL(48000.0) && sr_p != FL(96000.0))
        sr_p = FL(44100.0);

      if (UNLIKELY(sr != sr_p))
        csound->Message(csound,
                        Str("\n\nWARNING!!:\nOrchestra SR not compatible with "
                            "HRTF processing SR of: %.0f\n\n"), sr_p);

      /*  if (sr_p == FL(48000.0)) {
          sprintf(filel, "%s", "hrtf-48000-left.dat");
          sprintf(filer, "%s", "hrtf-48000-right.dat");
          }
          if (sr_p == FL(96000.0)) {
          sprintf(filel, "%s", "hrtf-96000-left.dat");
          sprintf(filer, "%s", "hrtf-96000-right.dat");
          }*/


      /* setup as per sr */
      if (sr_p == 44100 || sr_p == 48000)
        {
          irlength = 128;
          irlengthpad = 256;
          overlapsize = (irlength - 1);
        }
      else if (sr_p == 96000)
        {
          irlength = 256;
          irlengthpad = 512;
          overlapsize = (irlength - 1);
        }

      char filel[MAXNAME], filer[MAXNAME];

      /* copy in string name... */
      strncpy(filel, (char*) ifilel->data, MAXNAME-1); //filel[MAXNAME-1]='\0';
      strncpy(filer, (char*) ifiler->data, MAXNAME-1); //filel[MAXNAME-1]='\0';



      /* reading files, with byte swap */
      fpl = csound->LoadMemoryFile(csound, filel, CSFTYPE_FLOATS_BINARY,
                                     swap4bytes);
      if (UNLIKELY(fpl == NULL))
        return
          csound->InitError(csound, "%s",
                            Str("\n\n\nCannot load left data file, exiting\n\n"));

      fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,
                                     swap4bytes);
      if (UNLIKELY(fpr == NULL))
        return
          csound->InitError(csound,
                            "%s", Str("\n\n\nCannot load right data file, exiting\n\n"));

      irlength_p = irlength;
      irlengthpad_p = irlengthpad;
      overlapsize_p = overlapsize;

      sroverN_p = sr_p/irlength;

      /* start indices at correct value (start of file)/ zero indices.
         (do not need to store here, as only accessing in INIT) */
      fpindexl = (float *) fpl->beginp;
      fpindexr = (float *) fpr->beginp;
      ////
      //    /* buffers */
      if (!insig_p.auxp || insig_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &insig_p);
      if (!outl_p.auxp || outl_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &outl_p);
      if (!outr_p.auxp || outr_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &outr_p);
      if (!hrtflpad_p.auxp || hrtflpad_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &hrtflpad_p);
      if (!hrtfrpad_p.auxp || hrtfrpad_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &hrtfrpad_p);
      if (!complexinsig_p.auxp || complexinsig_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), & complexinsig_p);
      if (!hrtflfloat_p.auxp || hrtflfloat_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &hrtflfloat_p);
      if (!hrtfrfloat_p.auxp || hrtfrfloat_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &hrtfrfloat_p);
      if (!outspecl_p.auxp || outspecl_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &outspecl_p);
      if (!outspecr_p.auxp || outspecr_p.size < irlengthpad * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &outspecr_p);
      if (!overlapl_p.auxp || overlapl_p.size < overlapsize * sizeof(MYFLT))
        csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &overlapl_p);
      if (!overlapr_p.auxp || overlapr_p.size < overlapsize * sizeof(MYFLT))
        csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &overlapr_p);

      memset(insig_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(outl_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(outr_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(hrtflpad_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(hrtfrpad_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(complexinsig_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(hrtflfloat_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(hrtfrfloat_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(outspecl_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(outspecr_p.auxp, 0, irlengthpad * sizeof(MYFLT));
      memset(overlapl_p.auxp, 0, overlapsize * sizeof(MYFLT));
      memset(overlapr_p.auxp, 0, overlapsize * sizeof(MYFLT));

      /* interpolation values */
      if (!lowl1_p.auxp || lowl1_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &lowl1_p);
      if (!lowr1_p.auxp || lowr1_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &lowr1_p);
      if (!lowl2_p.auxp || lowl2_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &lowl2_p);
      if (!lowr2_p.auxp || lowr2_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &lowr2_p);
      if (!highl1_p.auxp || highl1_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &highl1_p);
      if (!highr1_p.auxp || highr1_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &highr1_p);
      if (!highl2_p.auxp || highl2_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &highl2_p);
      if (!highr2_p.auxp || highr2_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &highr2_p);

      /* best to zero, for future changes (filled in init) */
      memset(lowl1_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(lowr1_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(lowl2_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(lowr2_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(highl1_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(highl2_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(highr1_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(highr2_p.auxp, 0, irlength * sizeof(MYFLT));

      /* shift buffers */
      if (!leftshiftbuffer_p.auxp ||
          leftshiftbuffer_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &leftshiftbuffer_p);
      if (!rightshiftbuffer_p.auxp ||
          rightshiftbuffer_p.size < irlength * sizeof(MYFLT))
        csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &rightshiftbuffer_p);

      memset(leftshiftbuffer_p.auxp, 0, irlength * sizeof(MYFLT));
      memset(rightshiftbuffer_p.auxp, 0, irlength * sizeof(MYFLT));

      lowl1 = (MYFLT *)lowl1_p.auxp;
      lowr1 = (MYFLT *)lowr1_p.auxp;
      lowl2 = (MYFLT *)lowl2_p.auxp;
      lowr2 = (MYFLT *)lowr2_p.auxp;
      highl1 = (MYFLT *)highl1_p.auxp;
      highr1 = (MYFLT *)highr1_p.auxp;
      highl2 = (MYFLT *)highl2_p.auxp;
      highr2 = (MYFLT *)highr2_p.auxp;

      leftshiftbuffer = (MYFLT *)leftshiftbuffer_p.auxp;
      rightshiftbuffer = (MYFLT *)rightshiftbuffer_p.auxp;

      hrtflfloat = (MYFLT *)hrtflfloat_p.auxp;
      hrtfrfloat = (MYFLT *)hrtfrfloat_p.auxp;

      hrtflpad = (MYFLT *)hrtflpad_p.auxp;
      hrtfrpad = (MYFLT *)hrtfrpad_p.auxp;

      if (r <= 0 || r > 15)
        r = FL(8.8);

      if (elev > FL(90.0))
        elev = FL(90.0);
      if (elev < FL(-40.0))
        elev = FL(-40.0);

      while(angle < FL(0.0))
        angle += FL(360.0);
      while(angle >= FL(360.0))
        angle -= FL(360.0);

      /* two nearest elev indices to avoid recalculating */
      elevindexstore = (elev - minelev) / elevincrement;
      elevindexlow = (int32_t)elevindexstore;

      if (elevindexlow < 13)
        elevindexhigh = elevindexlow + 1;
      /* highest index reached */
      else
        elevindexhigh = elevindexlow;

      /* get percentage value for interpolation */
      elevindexhighper = elevindexstore - elevindexlow;

      /* avoid recalculation */
      angleindexlowstore = angle / (FL(360.0) / elevationarray[elevindexlow]);
      angleindexhighstore = angle / (FL(360.0) / elevationarray[elevindexhigh]);

      /* 4 closest indices, 2 low and 2 high */
      angleindex1 = (int32_t)angleindexlowstore;

      angleindex2 = angleindex1 + 1;
      angleindex2 = angleindex2 % elevationarray[elevindexlow];

      angleindex3 = (int32_t)angleindexhighstore;

      angleindex4 = angleindex3 + 1;
      angleindex4 = angleindex4 % elevationarray[elevindexhigh];

      /* angle percentages for interp */
      angleindex2per = angleindexlowstore - angleindex1;
      angleindex4per = angleindexhighstore - angleindex3;

      /* read 4 nearest HRTFs */
      skip = 0;
      /* switch l and r */
      if (angleindex1 > elevationarray[elevindexlow] / 2)
        {
          for (i = 0; i < elevindexlow; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < (elevationarray[elevindexlow] - angleindex1); i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              lowl1[i] = fpindexr[skip + i];
              lowr1[i] = fpindexl[skip + i];
            }
        }
      else
        {
          for (i = 0; i < elevindexlow; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < angleindex1; i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              lowl1[i] = fpindexl[skip + i];
              lowr1[i] = fpindexr[skip + i];
            }
        }

      skip = 0;
      if (angleindex2 > elevationarray[elevindexlow] / 2)
        {
          for (i = 0; i < elevindexlow; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < (elevationarray[elevindexlow] - angleindex2); i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              lowl2[i] = fpindexr[skip + i];
              lowr2[i] = fpindexl[skip + i];
            }
        }
      else
        {
          for (i = 0; i < elevindexlow; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < angleindex2; i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              lowl2[i] = fpindexl[skip + i];
              lowr2[i] = fpindexr[skip + i];
            }
        }

      skip = 0;
      if (angleindex3 > elevationarray[elevindexhigh] / 2)
        {
          for (i = 0; i < elevindexhigh; i++)
            skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < (elevationarray[elevindexhigh] - angleindex3); i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              highl1[i] = fpindexr[skip + i];
              highr1[i] = fpindexl[skip + i];
            }
        }
      else
        {
          for (i = 0; i < elevindexhigh; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < angleindex3; i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              highl1[i] = fpindexl[skip + i];
              highr1[i] = fpindexr[skip + i];
            }
        }

      skip = 0;
      if (angleindex4 > elevationarray[elevindexhigh] / 2)
        {
          for (i = 0; i < elevindexhigh; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < (elevationarray[elevindexhigh] - angleindex4); i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              highl2[i] = fpindexr[skip + i];
              highr2[i] = fpindexl[skip + i];
            }
        }
      else
        {
          for (i = 0; i < elevindexhigh; i++)
            skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
          for (i = 0; i < angleindex4; i++)
            skip += irlength;
          for (i = 0; i < irlength; i++)
            {
              highl2[i] = fpindexl[skip + i];
              highr2[i] = fpindexr[skip + i];
            }
        }

      /* woodworth process */
      /* ITD formula, check which ear is relevant to calculate angle from */
      if (angle > FL(180.0))
        radianangle = (angle - FL(180.0)) * PI_F / FL(180.0);
      else
        radianangle = angle * PI_F / FL(180.0);
      /* degrees to radians */
      radianelev = elev * PI_F / FL(180.0);

      /* get in correct range for formula */
      if (radianangle > PI_F / FL(2.0))
        radianangle = FL(PI) - radianangle;

      /* woodworth formula for itd */
      itdww = (radianangle + SIN(radianangle)) * r * COS(radianelev) / 34400.0;

      /* 0 Hz and Nyq... */
      /* these are real values...may be neg (implying phase of pi:
         in phase truncation), so need fabs... */
      magllow = FABS(lowl1[0]) + (FABS(lowl2[0]) - FABS(lowl1[0])) * angleindex2per;
      maglhigh = FABS(highl1[0]) + (FABS(highl2[0]) - FABS(highl1[0])) *
        angleindex4per;
      hrtflfloat[0] = magllow + (maglhigh - magllow) * elevindexhighper;

      magllow = FABS(lowl1[1]) + (FABS(lowl2[1]) - FABS(lowl1[1])) * angleindex2per;
      maglhigh = FABS(highl1[1]) + (FABS(highl2[1]) - FABS(highl1[1])) *
        angleindex4per;
      hrtflfloat[1] = magllow + (maglhigh - magllow) * elevindexhighper;

      magrlow = FABS(lowr1[0]) + (FABS(lowr2[0]) - FABS(lowr1[0])) * angleindex2per;
      magrhigh = FABS(highr1[0]) + (FABS(highr2[0]) - FABS(highr1[0])) *
        angleindex4per;
      hrtfrfloat[0] = magrlow + (magrhigh - magrlow) * elevindexhighper;

      magrlow = FABS(lowr1[1]) + (FABS(lowr2[1]) - FABS(lowr1[1])) * angleindex2per;
      magrhigh = FABS(highr1[1]) + (FABS(highr2[1]) - FABS(highr1[1])) *
        angleindex4per;
      hrtfrfloat[1] = magrlow + (magrhigh - magrlow) * elevindexhighper;

      /* magnitude interpolation */
      for (i = 2; i < irlength; i+=2)
        {
          /* interpolate high and low mags */
          magllow = lowl1[i] + (lowl2[i] - lowl1[i]) * angleindex2per;
          maglhigh = highl1[i]+(highl2[i] - highl1[i]) * angleindex4per;

          magrlow = lowr1[i] + (lowr2[i] - lowr1[i]) * angleindex2per;
          magrhigh = highr1[i] + (highr2[i] - highr1[i]) * angleindex4per;

          /* interpolate high and low results */
          magl = magllow + (maglhigh - magllow) * elevindexhighper;
          magr = magrlow + (magrhigh - magrlow) * elevindexhighper;

          freq = (i / 2) * sroverN_p;

          /* non linear itd...last value in array = 1.0, so back to itdww */
          if (sr_p == 96000)
            {
              if ((i / 2) < 6)
                itd = itdww * nonlinitd96k[(i / 2) - 1];
            }
          else if (sr_p == 48000)
            {
              if ((i / 2) < 6)
                itd = itdww * nonlinitd48k[(i / 2) - 1];
            }
          else if (sr_p == 44100)
            {
              if ((i / 2) < 6)
                itd = itdww * nonlinitd[(i / 2) - 1];
            }

          if (angle > FL(180.))
            {
              phasel = TWOPI_F * freq * (itd / 2);
              phaser = TWOPI_F * freq * -(itd / 2);
            }
          else
            {
              phasel = TWOPI_F * freq * -(itd / 2);
              phaser = TWOPI_F * freq * (itd / 2);
            }

          /* polar to rectangular */
          hrtflfloat[i] = magl * COS(phasel);
          hrtflfloat[i+1] = magl * SIN(phasel);

          hrtfrfloat[i] = magr * COS(phaser);
          hrtfrfloat[i+1] = magr * SIN(phaser);
        }

       setup_pad = csound->RealFFTSetup(csound, irlengthpad_p, FFT_FWD);
       setup = csound->RealFFTSetup(csound, irlength_p, FFT_FWD);
       isetup_pad = csound->RealFFTSetup(csound, irlengthpad_p, FFT_INV);
       isetup = csound->RealFFTSetup(csound, irlength_p, FFT_INV);

      /* ifft */
      csound->RealFFT(csound, isetup, hrtflfloat);
      csound->RealFFT(csound, isetup, hrtfrfloat);

      for (i = 0; i < irlength; i++)
        {
          /* scale and pad buffers with zeros to fftbuff */
          leftshiftbuffer[i] = hrtflfloat[i];
          rightshiftbuffer[i] = hrtfrfloat[i];
        }



      /* shift for causality...impulse as is is centred around zero time lag...
         then phase added. */
      /* this step centres impulse around centre tap of filter (then phase
         moves it for correct itd...) */
      shift = irlength / 2;

      for (i = 0; i < irlength; i++)
        {
          hrtflpad[i] = leftshiftbuffer[shift];
          hrtfrpad[i] = rightshiftbuffer[shift];

          shift++;
          shift = shift % irlength;
        }

      /* zero pad impulse */
      for (i = irlength; i < irlengthpad; i++)
        {
          hrtflpad[i] = FL(0.0);
          hrtfrpad[i] = FL(0.0);
        }

      /* back to freq domain */
      csound->RealFFT(csound, setup_pad, hrtflpad);
      csound->RealFFT(csound, setup_pad, hrtfrpad);

      /* initialize counter */
      counter_p = 0;

      return OK;
  }


  virtual int32_t hrtfstat_process(CSOUND *csound, MYFLT *in, MYFLT *outsigl, MYFLT *outsigr, uint32_t offset, uint32_t early, uint32_t nsmps)
  {
      /* local pointers to p */
      /*MYFLT *in = p->in->data;
        MYFLT *outsigl  = p->outsigl;
        MYFLT *outsigr = p->outsigr;*/

      /* common buffers and variables */
      MYFLT *insig = (MYFLT *)insig_p.auxp;
      MYFLT *outl = (MYFLT *)outl_p.auxp;
      MYFLT *outr = (MYFLT *)outr_p.auxp;

      MYFLT *hrtflpad = (MYFLT *)hrtflpad_p.auxp;
      MYFLT *hrtfrpad = (MYFLT *)hrtfrpad_p.auxp;

      MYFLT *complexinsig = (MYFLT *)complexinsig_p.auxp;
      MYFLT *outspecl = (MYFLT *)outspecl_p.auxp;
      MYFLT *outspecr = (MYFLT *)outspecr_p.auxp;

      MYFLT *overlapl = (MYFLT *)overlapl_p.auxp;
      MYFLT *overlapr = (MYFLT *)overlapr_p.auxp;

      int32_t counter = counter_p;
      int32_t i;
      //uint32_t offset = p->h.insdshead->ksmps_offset;
      //uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t j;//, nsmps = CS_KSMPS;

      int32_t irlength = irlength_p;
      int32_t irlengthpad = irlengthpad_p;
      int32_t overlapsize = overlapsize_p;

      MYFLT sr = sr_p;

      /* if (UNLIKELY(offset)) {
         memset(outsigl, '\0', offset*sizeof(MYFLT));
         memset(outsigr, '\0', offset*sizeof(MYFLT));
         }
         if (UNLIKELY(early)) {
         nsmps -= early;
         memset(&outsigl[nsmps], '\0', early*sizeof(MYFLT));
         memset(&outsigr[nsmps], '\0', early*sizeof(MYFLT));
         }*/
      for (j = offset; j < nsmps; j++)
        {
          /* ins and outs */
          insig[counter] = in[j];

          outsigl[j] = outl[counter];
          outsigr[j] = outr[counter];

          counter++;

          if (counter == irlength)
            {
              /* process a block */
              /* look after overlap add stuff */
              for (i = 0; i < overlapsize ; i++)
                {
                  overlapl[i] = outl[i+irlength];
                  overlapr[i] = outr[i+irlength];
                }

              /* insert insig for complex real,im fft, zero pad */
              for (i = 0; i <  irlength; i++)
                complexinsig[i] = insig[i];

              for (i = irlength; i <  irlengthpad; i++)
                complexinsig[i] = FL(0.0);

              csound->RealFFT(csound, setup_pad, complexinsig);

              /* complex multiplication */
              csound->RealFFTMult(csound, outspecl, hrtflpad, complexinsig,
                                  irlengthpad, FL(1.0));
              csound->RealFFTMult(csound, outspecr, hrtfrpad, complexinsig,
                                  irlengthpad, FL(1.0));

              /* convolution is the inverse FFT of above result */
              csound->RealFFT(csound, isetup_pad, outspecl);
              csound->RealFFT(csound, isetup_pad, outspecr);

              /* scaled by a factor related to sr...? */
              for (i = 0; i < irlengthpad; i++)
                {
                  outl[i] = outspecl[i] / (sr / FL(38000.0));
                  outr[i] = outspecr[i] / (sr / FL(38000.0));
                }

              for (i = 0; i < irlength; i++)
                {
                  outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
                  outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
                }

              /* reset counter */
              counter = 0;

            }       /* end of irlength == counter */

        }   /* end of ksmps audio loop */

      /* update */
      counter_p = counter;

      return OK;
  }
};


/* Csound structure for HOAMBDEC opcode */

typedef struct {
  OPDS h;

  ARRAYDAT*    out;        /* output buffers   */
  MYFLT*    setup;         /* configuration         */
  ARRAYDAT*    in;         /* input buffers    */
  MYFLT*    band;     // 0 for mix decoder, 1 for LF decoder, 2 for HF decoder
  MYFLT*    r;        // Distance for NFC. If r=-1 NFC off.
  MYFLT*    freq_cut; // frequency of band-splitting
  MYFLT*    type_mix; // 0 for energy, 1 for rms, 2 for amplitude
  STRINGDAT* ifilel;
  STRINGDAT* ifiler;

  int32_t numa;         /* i-var p-time storage registers */
  int32_t numb;

  /* band splitting coefficients */
  double a[MAXPOLES];
  double b_lf[MAXZEROS+1];
  double b_hf[MAXZEROS+1];

  /* matrices for LF and HF decoders */
  double     M_lf[MAX_OUTPUTS][MAX_INPUTS];
  double     M_hf[MAX_OUTPUTS][MAX_INPUTS];

  AUXCH delay[MAX_INPUTS];     /* delay-line state memory base pointer */
  double *currPos[MAX_INPUTS];  /* delay-line current position pointer */ /* >>Was float<< */
  int32_t   ndelay;    /* length of delay line (i.e. filter order) */

  // NFC temp variables
  float fRec2[MAX_INPUTS][2];
  float fRec0[MAX_INPUTS][2];
  float fRec3[MAX_INPUTS][2];
  float fRec5[MAX_INPUTS][2];
  float fRec6[MAX_INPUTS][2];
  float fRec8[MAX_INPUTS][2];
  float fRec9[MAX_INPUTS][2];
  float fRec11[MAX_INPUTS][2];
  float fRec12[MAX_INPUTS][2];
  float fRec14[MAX_INPUTS][2];

  int32_t order; // order
  bool horizontal; // true if the configuration is horizontal
  int32_t n_signals; // number of ambisonics signals

  hrtf* binaural[20]; // instances of hrtf class
  AUXCH     binaural_mem[20]; // aux for hrtf
  AUXCH     out_A;
  AUXCH     out_binaural0, out_binaural1;

} HOAMBDEC;

typedef struct FCOMPLEX {double r,i;} fcomplex;

static double readFilter(HOAMBDEC*, int32_t, int32_t);
static void insertFilter(HOAMBDEC*,double, int32_t);
static void process_nfc(CSOUND*,HOAMBDEC*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);

#ifndef MAX
#define MAX(a,b) ((a>b)?(a):(b))
//#define MIN(a,b) ((a>b)?(b):(a))
#endif

/*#define POLEISH (1) */     /* 1=poleish pole roots after Laguer root finding */

typedef struct FPOLAR {double mag,ph;} fpolar;

/* hoambdec initialization routine */
static int32_t ihoambdec(CSOUND *csound, HOAMBDEC* p)
{

    // Not used int32_t i;

    /* since i-time arguments are not guaranteed to propegate to p-time
     * we must copy the i-vars into the p structure.
     */

    // int32_t n1;
    // bool nfc = false;

    /* numbers of a and b coefficients */
    p->numa = 2;
    p->numb = 3;

    /* First check bounds on initialization arguments */
    if (UNLIKELY((p->numb<1) || (p->numb>(MAXZEROS+1)) ||
                 (p->numa<0) || (p->numa>MAXPOLES)))
      return csound->InitError(csound,  "%s", Str("Filter order out of bounds: "
                                                  "(1 <= nb < 51, 0 <= na <= 50)"));

    /* Calculate the total delay in samples and allocate memory for it */
    p->ndelay = MAX(p->numb-1,p->numa);

    int32_t type_mix = (int)*(p->type_mix);

    int32_t n_outs = p->out->sizes[0];
    int32_t n_ins = p->in->sizes[0];
    //char buffer [50];

    int32_t isetup = (int)*(p->setup);
    // int32_t order;

    if (isetup == 21)
      n_outs = 8;
    if (isetup == 31)
      n_outs = 20;

    switch (n_ins) {
    case 4: p->order = 1; break;
    case 9: p->order = 2; break;
    case 16: p->order = 3; break;
    case 25: p->order = 4; break;
    case 36: p->order = 5; break;
    default : return csound->InitError(csound, "%s", Str("illegal number of inputs"));
    }

    if ((isetup == 1) & (p->order >= 2)) {
      return csound->InitError(csound, "%s", Str("Stereo configuration only works with first order"));
    }
    if ((isetup == 2) & (p->order >= 2)) {
      return csound->InitError(csound, "%s", Str("Quad configuration only works with first order"));
    }

    if ((isetup == 3) & (p->order >= 3)) {
      return csound->InitError(csound, "%s", Str("5.0 configuration only works with first and second order"));
    }

    if ((isetup == 4) & (p->order >= 4)) {
      return csound->InitError(csound, "%s", Str("Octagon configuration only works with first, second and third order"));
    }

    if ((isetup == 5) & (p->order >= 2)) {
      return csound->InitError(csound, "%s", Str("Cube configuration only works with first order"));
    }

    if ((isetup == 6) & (p->order >= 3)) {
      return csound->InitError(csound, "%s", Str("Hexagon configuration only works with first and second order"));
    }

    if ((isetup == 21) & (p->order >= 4)) {
      return csound->InitError(csound, "%s", Str("2D Binaural configuration only works with first, second and third order"));
    }

    if ((isetup == 31) & (p->order >= 4)) {
      return csound->InitError(csound, "%s", Str("3D Binaural configuration only works with first, second and third order"));
    }

    p->horizontal = ((isetup == 1) | (isetup == 2) | (isetup == 3)  | (isetup == 4)  | (isetup == 6) | (isetup == 21));

    if (p->horizontal)
      p->n_signals = 2*p->order + 1;  //2l+1
    else
      p->n_signals = n_ins;   //(l+1)^2


    for (int32_t j = 0; j < n_ins; j++) {
      csound->AuxAlloc(csound, p->ndelay * sizeof(double), &p->delay[j]);
    }

    /* Set current position pointer to beginning of delay */

    for (int32_t j = 0; j < n_ins; j++) {
      p->currPos[j] = (double*)p->delay[j].auxp;
    }

    // Band-splitting filters coefficients
    double freq;
    if ((int)*(p->freq_cut) == 0) {
      freq = 400;
    } else {
      freq = (double)*(p->freq_cut);
    }


    double k = tan(freq * PI / CS_ESR);
    double k2 = (k*k + 2*k + 1);

    double b0_lf = k*k/k2; //b0
    double b1_lf = 2*b0_lf; //b1
    double b2_lf = b0_lf; //b2

    double b0_hf = 1/k2; //b0
    double b1_hf = -2*b0_hf; //b1
    double b2_hf = b0_hf; //b2

    double a1 = 2*(k*k - 1)/k2; //a1
    double a2 = (k*k - 2*k + 1)/k2; //a2

    p->b_lf[0] = b0_lf; //b0
    p->b_lf[1] = b1_lf; //b1
    p->b_lf[2] = b2_lf; //b2

    p->b_hf[0] = b0_hf; //b0
    p->b_hf[1] = b1_hf; //b1
    p->b_hf[2] = b2_hf; //b2

    p->a[0] = a1; //a1
    p->a[1] = a2; //a2

    double const1,const2,const3,const4,const5,const6,const7,const8,const9;

    if (isetup == 2) { // Quad order 1

      const1 = 0.3535533906;

      double M_lf[4][4] = { { const1, const1, const1, 0.0 },
                            { const1, -const1, const1, 0.0 },
                            { const1, -const1, -const1, 0.0 },
                            { const1, const1, -const1, 0.0 } };

      for (int32_t i = 0; i < 4; i++) {
        for (int32_t j = 0; j < 4; j++) {
          p->M_lf[i][j] = M_lf[i][j];
        }
      }


      double gW, gXYZ;

      switch (type_mix) {
      case 0: // "energy"
        gW = sqrt(2.0);
        gXYZ = 1.0;
        break;
      case 1: // "rms"
        gW = 1.0;
        gXYZ = sqrt(2.0)/2.0;
        break;
      case 2: // "amp"
        gW = 1.0;
        gXYZ = sqrt(2.0)/2.0;
        break;
      }

      for (int32_t j = 0; j < n_outs; j++) {
        p->M_hf[j][0] = p->M_lf[j][0]*gW;
        p->M_hf[j][1] = p->M_lf[j][1]*gXYZ;
        p->M_hf[j][2] = p->M_lf[j][2]*gXYZ;
        p->M_hf[j][3] = p->M_lf[j][3]*gXYZ;
      }
    }

    if (isetup == 3) { // 5.0 order 1
      // Decoders matrix

      if (p->order == 1) {

        // (  0.1433292786,  0.2408456747,  0.2206488057)
        // (  0.1433292786,  0.2408456747, -0.2206488057)
        // (  0.1023776971,  0.3115414566,            -0)
        // (   0.512588654,  -0.396616403,   0.414684109)
        // (   0.512588654,  -0.396616403,  -0.414684109)

        const1 = 0.1433292786;
        const2 = 0.2408456747;
        const3 = 0.2206488057;
        const4 = 0.1023776971;
        const5 = 0.3115414566;
        const6 = 0.512588654;
        const7 = 0.396616403;
        const8 = 0.414684109;

        double M_lf[5][4] = { { const1, const2, const3, 0.0 },
                              { const1, const2, -const3, 0.0 },
                              { const4, const5, 0.0, 0.0 },
                              { const6, -const7, const8, 0.0 },
                              { const6, -const7, -const8, 0.0 } };

        for (int32_t i = 0; i < 5; i++) {
          for (int32_t j = 0; j < 4; j++) {
            p->M_lf[i][j] = M_lf[i][j];
          }
        }

        double gW, gXYZ;

        switch (type_mix) {
        case 0: // "energy"
          gW = 1.58113883;
          gXYZ = 1.118033989;
          break;
        case 1: // "rms"
          gW = 1.0;
          gXYZ = sqrt(2.0)/2.0;
          break;
        case 2: // "amp"
          gW = 1.0;
          gXYZ = sqrt(2.0)/2.0;
          break;
        }

        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*gW;
          p->M_hf[j][1] = p->M_lf[j][1]*gXYZ;
          p->M_hf[j][2] = p->M_lf[j][2]*gXYZ;
          p->M_hf[j][3] = p->M_lf[j][3]*gXYZ;
        }
      }

      if (p->order == 2) {

        // ( -0.6902095308,   2.032716615,  0.2831185829,  -1.544664776,  0.4138916792);
        // ( -0.6902095308,   2.032716615, -0.2831185829,  -1.544664776, -0.4138916792);
        // (   1.603030056,  -2.914433535,            -0,   2.780920112,            -0);
        // (  0.5958012839, -0.5754998474,  0.3814446348,  0.1542047194, -0.2202271626);
        // (  0.5958012839, -0.5754998474, -0.3814446348,  0.1542047194,  0.2202271626);

        const1 = 0.6902095308;
        const2 = 2.032716615;
        const3 = 0.2831185829;
        const4 = 1.544664776;
        const5 = 0.4138916792;
        const6 = 1.603030056;
        const7 = 2.914433535;
        const8 = 2.780920112;
        const9 = 0.5958012839;
        double const10 = 0.5754998474;
        double const11 = 0.3814446348;
        double const12 = 0.1542047194;
        double const13 = 0.2202271626;

        double M_lf[5][5] = { { -const1, const2, const3, -const4, const5 },
                              { -const1, const2, -const3, -const4, -const5 },
                              { const6, -const7, 0.0, const8 ,0.0 },
                              { const9, -const10, const11, const12, -const13 },
                              { const9, -const10, -const11, const12, const13 } };

        for (int32_t i = 0; i < 5; i++) {
          for (int32_t j = 0; j < 5; j++) {
            p->M_lf[i][j] = M_lf[i][j];
          }
        }

        double g0, g1, g2;

        switch (type_mix) {
        case 0: // "energy"
          g0 = 1.2909944487;
          g1 = 1.1180339888;
          g2 = 0.6454972244;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = 0.86602540378;
          g2 = 0.5;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.86602540378;
          g2 = 0.5;
          break;
        }

        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g2;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
        }


      }
    }

    if (isetup == 4 || isetup == 21) { // Octagon or binaural 2D

      // Decoders matrix
      //0.1767766953,  0.2309698831,  0.0956708581,  0.1767766953,  0.1767766953,  0.0956708581,  0.2309698831
      //  0.1767766953,  0.0956708581,  0.2309698831, -0.1767766953,  0.1767766953, -0.2309698831, -0.0956708581
      // 0.1767766953, -0.0956708581,  0.2309698831, -0.1767766953, -0.1767766953,  0.2309698831, -0.0956708581
      // 0.1767766953, -0.2309698831,  0.0956708581,  0.1767766953, -0.1767766953, -0.0956708581,  0.2309698831
      //0.1767766953, -0.2309698831, -0.0956708581,  0.1767766953,  0.1767766953, -0.0956708581, -0.2309698831
      //0.1767766953, -0.0956708581, -0.2309698831, -0.1767766953,  0.1767766953,  0.2309698831,  0.0956708581
      // 0.1767766953,  0.0956708581, -0.2309698831, -0.1767766953, -0.1767766953, -0.2309698831,  0.0956708581
      // 0.1767766953,  0.2309698831, -0.0956708581,  0.1767766953, -0.1767766953,  0.0956708581, -0.2309698831
      const1 = 0.1767766953;
      const2 = 0.2309698831;
      const3 = 0.0956708581;

      double M_lf[8][7] = { { const1, const2, const3, const1, const1, const3, const2 },
                            { const1, const3, const2, -const1, const1, -const2, -const3 },
                            { const1, -const3, const2, -const1, -const1, const2, -const3 },
                            { const1, -const2, const3, const1, -const1, -const3, const2 },
                            { const1, -const2, -const3, const1, const1, -const3, -const2 },
                            { const1, -const3, -const2, -const1, const1, const2, const3 },
                            { const1, const3, -const2, -const1, -const1, -const2, const3 },
                            { const1, const2, -const3, const1, -const1, const3, -const2 } };

      for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 0; j < 7; j++) {
          p->M_lf[i][j] = M_lf[i][j];
        }
      }


      double g0, g1, g2, g3;

      if (p->order == 1) { //order 1

        switch (type_mix) {
        case 0: // "energy"
          g0 = 2.0;
          g1 = sqrt(2);
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = sqrt(2.0)/2.0;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = sqrt(2.0)/2.0;
          break;
        }


        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
        }

      }

      if (p->order == 2) { //order 2

        switch (type_mix) {
        case 0: // "energy"
          g0 = 1.632993162;
          g1 = 1.414213562;
          g2 = 0.8164965809;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = 0.8660254038;
          g2 = 0.5;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.8660254038;
          g2 = 0.5;
          break;
        }

        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g2;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
        }

      }

      if (p->order == 3) { //order 3


        switch (type_mix) {
        case 0: // "energy"
          g0 = sqrt(2.0);
          g1 = 1.306562965;
          g2 = 1.0;
          g3 = 0.5411961001;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = 0.923879533;
          g2 = 0.707106781;
          g3 = 0.382683432;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.923879533;
          g2 = 0.707106781;
          g3 = 0.382683432;
          break;
        }


        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g2;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
          p->M_hf[j][5] = p->M_lf[j][5]*g3;
          p->M_hf[j][6] = p->M_lf[j][6]*g3;
        }
      }
    }

    if (isetup == 5) { // Cube order 1
      const1 = 0.1767766953;
      const2 = 0.2165063509;

      // Decoders matrix
      //(  0.1767766953,  0.2165063509,  0.2165063509)
      //(  0.1767766953,  0.2165063509,  0.2165063509)
      //(  0.1767766953, -0.2165063509,  0.2165063509)
      // (  0.1767766953, -0.2165063509,  0.2165063509)
      // (  0.1767766953, -0.2165063509, -0.2165063509)
      // (  0.1767766953, -0.2165063509, -0.2165063509)
      // (  0.1767766953,  0.2165063509, -0.2165063509)
      //(  0.1767766953,  0.2165063509, -0.2165063509)

      double M_lf[8][4] = { { const1, const2, const2, 0.0 },
                            { const1, const2, const2, 0.0 },
                            { const1, -const2, const2, 0.0 },
                            { const1, -const2, const2, 0.0 },
                            { const1, -const2, -const2, 0.0 },
                            { const1, -const2, -const2, 0.0 },
                            { const1, const2, -const2, 0.0 },
                            { const1, const2, -const2, 0.0 } };

      for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 0; j < 4; j++) {
          p->M_lf[i][j] = M_lf[i][j];
        }
      }

      double gW, gXYZ;

      switch (type_mix) {
      case 0: // "energy"
        gW = 2.0;
        gXYZ = sqrt(2.0);
        break;
      case 1: // "rms"
        gW = 1.0;
        gXYZ = sqrt(2.0)/2.0;
        break;
      case 2: // "amp"
        gW = 1.0;
        gXYZ = sqrt(2.0)/2.0;
        break;
      }


      for (int32_t j = 0; j < n_outs; j++) {
        p->M_hf[j][0] = p->M_lf[j][0]*gW;
        p->M_hf[j][1] = p->M_lf[j][1]*gXYZ;
        p->M_hf[j][2] = p->M_lf[j][2]*gXYZ;
        p->M_hf[j][3] = p->M_lf[j][3]*gXYZ;
      }
    }


    if (isetup == 6) { // hexagon

      // Decoders matrix
      //(  0.2357022604,  0.2886751346,  0.1666666667,  0.1666666667,  0.2886751346);
      //(  0.2357022604,             0,  0.3333333333, -0.3333333333,            -0);
      //(  0.2357022604, -0.2886751346,  0.1666666667,  0.1666666667, -0.2886751346);
      //(  0.2357022604, -0.2886751346, -0.1666666667,  0.1666666667,  0.2886751346);
      //(  0.2357022604,             0, -0.3333333333, -0.3333333333,             0);
      //(  0.2357022604,  0.2886751346, -0.1666666667,  0.1666666667, -0.2886751346);

      const1 = 0.2357022604;
      const2 = 0.2886751346;
      const3 = 0.1666666667;
      const4 = 0.3333333333;

      double M_lf[6][5] = { { const1, const2, const3, const3, const2},
                            { const1, 0.0, const4, -const4, 0.0 },
                            { const1, -const2, const3, const3, -const2 },
                            { const1, -const2, -const3, const3, const2 },
                            { const1, 0.0, -const4, -const4, 0.0 },
                            { const1, const2, -const3, const3, -const2 } };

      for (int32_t i = 0; i < 6; i++) {
        for (int32_t j = 0; j < 5; j++) {
          p->M_lf[i][j] = M_lf[i][j];
        }
      }


      double g0, g1, g2;

      if (p->order == 1) { //order 1

        switch (type_mix) {
        case 0: // "energy"
          g0 =  1.732050808;
          g1 = 1.2247448714;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = sqrt(2.0)/2.0;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = sqrt(2.0)/2.0;
          break;
        }


        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
        }

      }

      if (p->order == 2) { //order 2

        switch (type_mix) {
        case 0: // "energy"
          g0 = 1.414213562;
          g1 = 1.224744871;
          g2 = 0.707106781;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = 0.8660254038;
          g2 = 0.5;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.8660254038;
          g2 = 0.5;
          break;
        }

        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g2;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
        }

      }
    }


    if (isetup == 31) { // binaural 3D (dodecahedron)
      // Decoders matrix
      const1 = 0.0707106781;
      const2 = 0.0866025404;
      const3 = 0.125;
      const4 = 0.1948557159;
      const5 = 0.024376941;
      const6 = 0.225623059;
      const7 = 0.1397542486;
      const8 = 0.2125578696;
      const9 = 0.0310117752;

      double const10 = 0.1401258538;
      double const11 = 0.0535233135;
      double const12 = 0.0658359214;
      double const13 = 0.3341640786;
      double const14 = 0.1623797632;

      double const15 = 0.0772542486;
      double const16 = 0.1636271243;
      double const17 = 0.0802849702;
      double const18 = 0.151246118;
      double const19 = 0.3272542486;
      double const20 = 0.1003562127;

      double const21 = 0.2022542486;
      double const22 = 0.0238728757;
      double const23 = 0.2101887808;
      double const24 = 0.251246118;
      double const25 = 0.0477457514;
      double const26 = 0.262735976;
      //0.0707106781,  0.0866025404,  0.0866025404, 0.0866025404,  -0, 0.125, 0.125,-0, 0.125, -0.1948557159,  -0.024376941,  0.225623059,  0.1397542486,0.125, -0.2125578696,  0.0310117752);
      // 0.0707106781,  0.0866025404, -0.0866025404,  0.0866025404,  0, 0.125,  -0.125, 0,-0.125, -0.1948557159,  -0.024376941,  -0.225623059,  0.1397542486, -0.125, -0.2125578696, -0.0310117752);
      // 0.0707106781, -0.0866025404, -0.0866025404,  0.0866025404, -0,-0.125, -0.125,-0, 0.125, -0.1948557159,   0.024376941,-0.225623059,  0.1397542486,0.125,  0.2125578696, -0.0310117752
      //0.0707106781, -0.0866025404,  0.0866025404,  0.0866025404, -0,-0.125,  0.125,0, -0.125, -0.1948557159,   0.024376941,  0.225623059,  0.1397542486, -0.125,  0.2125578696,  0.0310117752
      // 0.0707106781, 0.0866025404, 0.0866025404, -0.0866025404,0, -0.125,-0.125,0, 0.125,  0.1948557159,  -0.024376941, 0.225623059, -0.1397542486,-0.125, -0.2125578696,  0.0310117752
      // 0.0707106781,  0.0866025404, -0.0866025404, -0.0866025404,0,-0.125,0.125,-0, -0.125,  0.1948557159,  -0.024376941, -0.225623059, -0.1397542486,0.125, -0.2125578696, -0.0310117752
      // 0.0707106781, -0.0866025404, -0.0866025404, -0.0866025404, -0, 0.125, 0.125,  -0,  0.125,  0.1948557159,   0.024376941,  -0.225623059, -0.1397542486,-0.125,  0.2125578696, -0.0310117752
      // 0.0707106781, -0.0866025404,  0.0866025404, -0.0866025404,-0,  0.125,-0.125,-0,-0.125,  0.1948557159,   0.024376941,   0.225623059, -0.1397542486,0.125,  0.2125578696,  0.0310117752
      //0.0707106781,  0.1401258538,  0.0535233135,-0, -0.125, 0, -0,  0.1397542486, 0.125, 0, -0.0658359214, -0.3341640786, 0, -0,  0.1623797632,  0.1623797632
      //0.0707106781,  0.1401258538, -0.0535233135, -0, -0.125, 0, 0,  0.1397542486, -0.125, -0, -0.0658359214,  0.3341640786,-0, -0,  0.1623797632, -0.1623797632
      //0.0707106781, -0.1401258538, -0.0535233135,  0, -0.125, 0, -0,  0.1397542486, 0.125, -0,  0.0658359214,  0.3341640786, -0, 0, -0.1623797632, -0.1623797632
      //0.0707106781, -0.1401258538,  0.0535233135,  0, -0.125, -0, 0,  0.1397542486, -0.125, -0,  0.0658359214, -0.3341640786, -0, 0, -0.1623797632,  0.1623797632
      // 0.0707106781,  -0,  0.1401258538,  0.0535233135, -0.0772542486,   0,0.125, -0.1636271243,  0,  0.0802849702, 0,  -0.151246118, -0.3272542486,  0,-0, -0.1003562127
      // 0.0707106781, 0,  0.1401258538, -0.0535233135, -0.0772542486,  -0, -0.125, -0.1636271243,  0, -0.0802849702, 0,  -0.151246118,  0.3272542486, 0,  -0, -0.1003562127
      //0.0707106781, -0, -0.1401258538, -0.0535233135, -0.0772542486,  -0,  0.125, -0.1636271243,  -0, -0.0802849702,  -0,   0.151246118,  0.3272542486,   0,  0,  0.1003562127
      // 0.0707106781,  0, -0.1401258538,  0.0535233135, -0.0772542486, -0, -0.125, -0.1636271243, 0,  0.0802849702,-0,   0.151246118, -0.3272542486,  -0,-0,  0.1003562127
      // 0.0707106781,  0.0535233135, -0,  0.1401258538,  0.2022542486, 0.125, 0,  0.0238728757,  0,  0.2101887808,   0.251246118, -0, -0.0477457514, -0,   0.262735976,  -0
      // 0.0707106781,  0.0535233135,-0, -0.1401258538,  0.2022542486,  -0.125, -0,  0.0238728757,  -0, -0.2101887808,   0.251246118,  -0,  0.0477457514, 0,   0.262735976, -0
      //0.0707106781, -0.0535233135,  0, -0.1401258538,  0.2022542486,  0.125,  0,  0.0238728757, -0, -0.2101887808,  -0.251246118, 0,  0.0477457514,  0,  -0.262735976, 0
      // 0.0707106781, -0.0535233135, 0,  0.1401258538,  0.2022542486,  -0.125, -0,  0.0238728757,  -0,  0.2101887808,  -0.251246118, 0, -0.0477457514,  -0,  -0.262735976,0

      double M_lf[20][16] = { { const1, const2, const2, const2, 0.0, const3, const3, 0.0, const3, -const4, -const5, const6, const7, const3, -const8, const9 },
                              { const1, const2, -const2, const2, 0.0, const3, -const3, 0.0, -const3, -const4, -const5, -const6, const7, -const3, -const8, -const9 },
                              { const1, -const2, -const2, const2, 0.0, -const3, -const3, 0.0, const3, -const4, const5, -const6, const7, const3, const8, -const9 },
                              { const1, -const2, const2, const2, 0.0, -const3, const3, 0.0, -const3, -const4, const5, const6, const7, -const3, const8, const9 },
                              { const1, const2, const2, -const2, 0.0, -const3, -const3, 0.0, const3, const4, -const5, const6, -const7, -const3, -const8, const9 },
                              { const1, const2, -const2, -const2, 0.0, -const3, const3, 0.0, -const3, const4, -const5, -const6, -const7, const3, -const8, -const9 },
                              { const1, -const2, -const2, -const2, 0.0, const3, const3, 0.0, const3, const4, const5, -const6, -const7, -const3, const8, -const9 },
                              { const1, -const2, const2, -const2, 0.0, const3, -const3, 0.0, -const3, const4, const5, const6, -const7, const3, const8, const9 },
                              { const1, const10, const11, 0.0, -const3, 0.0, 0.0, const7, const3, 0.0, -const12, -const13, 0.0, 0.0, const14, const14 },
                              { const1, const10, -const11, 0.0, -const3, 0.0, 0.0, const7, -const3, 0.0, -const12, const13, 0.0, 0.0, const14, -const14 },
                              { const1, -const10, -const11, 0.0, -const3, 0.0, 0.0, const7, const3, 0.0, const12, const13, 0.0, 0.0, -const14, -const14 },
                              { const1, -const10, const11, 0.0, -const3, 0.0, 0.0, const7, -const3, 0.0, const12, -const13, 0.0, 0.0, -const14, const14 },
                              { const1, 0.0, const10, const11, -const15, 0.0, const3, -const16, 0.0, const17, 0.0, -const18, -const19, 0.0, 0.0, const20 },
                              { const1, 0.0, const10, -const11, -const15, 0.0, -const3, -const16, 0.0, -const17, 0.0, -const18, const19, 0.0, 0.0, -const20 },
                              { const1, 0.0, -const10, -const11, -const15, 0.0, const3, -const16, 0.0, -const17, 0.0, const18, const19, 0.0, 0.0, const20 },
                              { const1, 0.0, -const10, const11, -const15, 0.0, -const3, -const16, 0.0, const17, 0.0, const18, -const19, 0.0, 0.0, const20 },
                              { const1, const11, 0.0, const10, const21, const3, 0.0, const22, 0.0, const23, const24, 0.0, -const25, 0.0, const26, 0.0 },
                              { const1, const11, 0.0, -const10, const21, -const3, 0.0, const22, 0.0, -const23, const24, 0.0, const25, 0.0, const26, 0.0 },
                              { const1, -const11, 0.0, -const10, const21, const3, 0.0, const22, 0.0, -const23, const24, 0.0, const25, 0.0, -const26, 0.0 },
                              { const1, -const11, 0.0, const10, const21, -const3, 0.0, const22, 0.0, const23, -const24, 0.0, -const25, 0.0, -const26, 0.0 } };

      for (int32_t i = 0; i < 20; i++) {
        for (int32_t j = 0; j < 16; j++) {
          p->M_lf[i][j] = M_lf[i][j];
        }
      }

      double g0, g1, g2, g3;
      if (p->order == 1) { //order 1

        switch (type_mix) {
        case 0: // "energy"
          g0 = 3.16227766;
          g1 = 1.825741858;
          break;
        case 1: // "rms"
          g0 = 1.0;
          g1 = 0.577350269;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.577350269;
          break;
        }

        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g1;
        }

      }

      if (p->order == 2) { //order 2

        switch (type_mix) {
        case 0: // "energy"
          g0 = 2.357022604;
          g1 = 1.825741858;
          g2 = 0.9428090416;
          break;
        case 1: // "rms"
          g0 = 0.912870929;
          g1 = 0.707106781;
          g2 = 0.365148372;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.774596669;
          g2 = 0.4;
          break;
        }

        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g1;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
          p->M_hf[j][5] = p->M_lf[j][5]*g2;
          p->M_hf[j][6] = p->M_lf[j][6]*g2;
          p->M_hf[j][7] = p->M_lf[j][7]*g2;
          p->M_hf[j][8] = p->M_lf[j][8]*g2;
        }

      }

      if (p->order == 3) { //order 3

        switch (type_mix) {
        case 0: // "energy"
          g0 = 1.865086714;
          g1 = 1.606093894;
          g2 = 1.142055301;
          g3 = 0.5683795528;
          break;
        case 1: // "rms"
          g0 = 0.834092135;
          g1 = 0.718267025;
          g2 = 0.510742657;
          g3 = 0.254187063;
          break;
        case 2: // "amp"
          g0 = 1.0;
          g1 = 0.861136312;
          g2 = 0.612333621;
          g3 = 0.304746985;
          break;
        }

        // ( 0, 1, 1, 2, 2, 3, 3); ambisonic order of each input component
        for (int32_t j = 0; j < n_outs; j++) {
          p->M_hf[j][0] = p->M_lf[j][0]*g0;
          p->M_hf[j][1] = p->M_lf[j][1]*g1;
          p->M_hf[j][2] = p->M_lf[j][2]*g1;
          p->M_hf[j][3] = p->M_lf[j][3]*g1;
          p->M_hf[j][4] = p->M_lf[j][4]*g2;
          p->M_hf[j][5] = p->M_lf[j][5]*g2;
          p->M_hf[j][6] = p->M_lf[j][6]*g2;
          p->M_hf[j][7] = p->M_lf[j][7]*g2;
          p->M_hf[j][8] = p->M_lf[j][8]*g2;
          p->M_hf[j][9] = p->M_lf[j][9]*g3;
          p->M_hf[j][10] = p->M_lf[j][10]*g3;
          p->M_hf[j][11] = p->M_lf[j][11]*g3;
          p->M_hf[j][12] = p->M_lf[j][12]*g3;
          p->M_hf[j][13] = p->M_lf[j][13]*g3;
          p->M_hf[j][14] = p->M_lf[j][14]*g3;
          p->M_hf[j][15] = p->M_lf[j][15]*g3;
        }
      }
    }

    /* initialize binaural objects */

    if (isetup == 21) { //binaural 2D
      int32_t elev = 0;
      float angle[8] = {-22.5f,-67.5f,-112.5f,-157.5f,157.5f,112.5f,67.5f,22.5f};
      int32_t r = 1;

      for (int32_t j = 0; j < 8; j++) {
        if (p->binaural_mem[j].auxp == NULL)
          csound->AuxAlloc(csound, sizeof(hrtf_c), &p->binaural_mem[j]);
        p->binaural[j] = new (p->binaural_mem[j].auxp) hrtf_c;

        p->binaural[j]->hrtfstat_init(csound, elev, angle[j], r, p->ifilel, p->ifiler, CS_ESR);
      }
    }

    if (isetup == 31) { //binaural 3D
      float angle[20] = {-45.0f,45.0f,135.0f,-135.0f,-45.0f,45.0f,135.0f,-135.0f,-20.905157f,
                         20.905157f,159.094843f,-159.094843f,-90.0f,-90.0f,90.0f,
                         90.0f,0.0f,0.0f,-180.0f,-180.0f};
      float elev[20] = {35.264390f,35.264390f,35.264390f,35.264390f,
                        -35.264390f,-35.264390f,-35.264390f,-35.264390f,
                        0.0f,0.0f,0.0f,0.0f,
                        20.905157f,-20.905157f,-20.905157f,20.905157f,
                        69.094843f,-69.094843f,-69.094843f,69.094843f};
      int32_t r = 1;

      for (int32_t j = 0; j < 20; j++) {
        if (p->binaural_mem[j].auxp == NULL)
          csound->AuxAlloc(csound, sizeof(hrtf_c), &p->binaural_mem[j]);
        p->binaural[j] = new (p->binaural_mem[j].auxp) hrtf_c;
        p->binaural[j]->hrtfstat_init(csound, elev[j], angle[j], r, p->ifilel, p->ifiler, CS_ESR);
      }
    }

    for (int32_t l0 = 0; (l0 < 2); l0 = (l0 + 1)) {
      for (int32_t sig = 0; (sig < p->n_signals); sig = (sig + 1)) {
        p->fRec2[sig][l0] = 0.0;
        p->fRec0[sig][l0] = 0.0;
        p->fRec3[sig][l0] = 0.0;
        p->fRec5[sig][l0] = 0.0;
        p->fRec6[sig][l0] = 0.0;
        p->fRec8[sig][l0] = 0.0;
        p->fRec9[sig][l0] = 0.0;
        p->fRec11[sig][l0] = 0.0;
      }
    }
    uint32_t nsmps = CS_KSMPS;
    csound->AuxAlloc(csound, sizeof(MYFLT)*20*nsmps, &p->out_A);
    csound->AuxAlloc(csound, sizeof(MYFLT)*nsmps, &p->out_binaural0);
    csound->AuxAlloc(csound, sizeof(MYFLT)*nsmps, &p->out_binaural1);
    return OK;
}


/* hoambdec process routine */
static int32_t ahoambdec(CSOUND *csound, HOAMBDEC* p)
{
    IGN(csound);
    int32_t      i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    int32_t j;
    //char buffer [50];
    //int n1;
    int32_t ksmps = CS_KSMPS;
    int32_t n_outs = p->out->sizes[0];

    //int n_ins = p->in->sizes[0];
    int32_t isetup = (int)*(p->setup);

    MYFLT *in = p->in->data, *out = p->out->data;

    /* Outer loop */
    if (UNLIKELY(offset)) {
      for (j = 0; j < n_outs; j++) {
        memset(&out[j], '\0', offset*sizeof(MYFLT));
      }}
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (j = 0; j < n_outs; j++) {
        memset(&out[j*ksmps+nsmps], '\0', early*sizeof(MYFLT));
      }
    }

    // ***** FIX MEMORY ALOCATION *****
    //double poleSamp[p->n_signals],inSamp[p->n_signals];
    //double zeroSamp_lf[p->n_signals],zeroSamp_hf[p->n_signals];
    double poleSamp[36],inSamp[36];
    double zeroSamp_lf[36],zeroSamp_hf[36];
    //double poleSamp_nfc[p->n_signals],inSamp_nfc[p->n_signals],zeroSamp_nfc[p->n_signals];

    double y_lf,y_hf;

    //dict to convert 3d ambisonic signals in 2d. MAX for order 5
    int32_t dict_3dto2d[11] = { 0, 1, 2, 7, 8, 14, 15, 23, 24, 34, 35};
    int32_t order_signals3d[36]  = {0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5};
    int32_t order_signals2d[11]  = {0,1,1,2,2,3,3,4,4,5,5};

    int32_t n_outs_A;

    if (isetup == 21) { //binaural 2D
      n_outs_A = 8;
    } else if (isetup == 31) {  //binaural 3D
      n_outs_A = 20;
    } else {
      n_outs_A = n_outs;
    }

    // ***** FIX MEMORY ALOCATION *****
    //MYFLT out_A[20/*n_outs_A*/][nsmps-offset];
    //MYFLT out_binaural0[nsmps-offset],out_binaural1[nsmps-offset];
    MYFLT *out_A, *out_binaural0, *out_binaural1;
    out_A = (MYFLT*)p->out_A.auxp;
    out_binaural0 = (MYFLT*)p->out_binaural0.auxp;
    out_binaural1 = (MYFLT*)p->out_binaural1.auxp;

    for (n=offset; n<nsmps; n++) {

      if (isetup == 1) { // Stereo configuration. Nor band splitting nor near field compensation.
        /* Left: */
        out[n] = in[n]*SQRT(FL(0.5)) + in[2*ksmps+n]*FL(0.5);    // w*sqrt(0.5) + y*0.5;
        /* Right: */
        out[ksmps+n] = in[n]*SQRT(FL(0.5)) - in[2*ksmps+n]*FL(0.5);    // w*sqrt(0.5) - y*0.5;
        continue;
      }

      out_binaural0[n] = 0.0;
      out_binaural1[n] = 0.0;
      for (int32_t o = 0; o < n_outs; o++) {
        out[o*ksmps+n] = 0.0;
      }

      for (int32_t o = 0; o < n_outs_A; o++) {
        out_A[ksmps*o+n] = 0.0;
      }

      for (j = 0; j < p->n_signals; j++) {  // ambisonics signals

        int32_t in_ix;
        if (p->horizontal)
          in_ix = dict_3dto2d[j];
        else
          in_ix = j;

        int32_t signal_order;

        if  (p->horizontal)
          signal_order = order_signals2d[j];
        else
          signal_order = order_signals3d[j];

        if (signal_order != 0) {
          if ((int)*(p->r)!=-1)
            process_nfc(csound,p,signal_order,n,j,in_ix,CS_ESR,CS_KSMPS);
        }

        // band splitting
        inSamp[j] = in[in_ix*ksmps+n];
        poleSamp[j] = inSamp[j];
        zeroSamp_lf[j] = 0.0;
        zeroSamp_hf[j] = 0.0;

        /* Inner filter loop */
        for (i=0; i< p->ndelay; i++) {
          /* Do poles first */
          /* Sum of products of a's and delays */
          if (i<p->numa)
            poleSamp[j] += -(p->a[i])*readFilter(p,i+1,j);

          /* Now do the zeros */
          if (i<(p->numb-1)) {
            zeroSamp_lf[j] += (p->b_lf[i+1])*readFilter(p,i+1,j);
            zeroSamp_hf[j] += (p->b_hf[i+1])*readFilter(p,i+1,j);
          }
        }

        insertFilter(p, poleSamp[j], j);
        y_hf = (p->b_hf[0])*poleSamp[j] + zeroSamp_hf[j];
        y_lf = (p->b_lf[0])*poleSamp[j] + zeroSamp_lf[j];

        for (int32_t o = 0; o < n_outs_A; o++) {
          if (isetup == 21 || isetup == 31) { //binaural
            switch((int)*(p->band)) {
            case 0: out_A[o*ksmps+n] += (MYFLT) ((p->M_lf[o][in_ix])*y_lf - (p->M_hf[o][in_ix])*y_hf); break;
            case 1: out_A[o*ksmps+n] += (MYFLT) y_lf; break;
            case 2: out_A[o*ksmps+n] += (MYFLT) y_hf; break;
            default: ;
            }
          } else {
            switch((int)*(p->band)) {
            case 0: out[o*ksmps+n] += (MYFLT) ((p->M_lf[o][in_ix])*y_lf - (p->M_hf[o][in_ix])*y_hf); break;
            case 1: out[o*ksmps+n] += (MYFLT) y_lf; break;
            case 2: out[o*ksmps+n] += (MYFLT) y_hf; break;
            default: ;
            }
          }
        }
      }
    }

    if (isetup == 21 || isetup == 31) { //binaural 2D or 3D
      for (int32_t j = 0; j < n_outs_A; j++) {
        p->binaural[j]->hrtfstat_process(csound, &out_A[j*ksmps], out_binaural0, out_binaural1, offset, early, nsmps);

        for (n=offset; n<nsmps; n++) {
          out[n] += (MYFLT) out_binaural0[n];
          out[ksmps+n] += (MYFLT) out_binaural1[n];
        }
      }
    }

    return OK;
}


/* readFilter -- delay-line access routine (band splitting)
 *
 * Reads sample x[n-i] from a previously established delay line.
 * With this syntax i is +ve for a time delay and -ve for a time advance.
 *
 * The use of explicit indexing rather than implicit index incrementing
 * allows multiple lattice structures to access the same delay line.
 * (adapted from  csound/Opcodes/hrtfopcodes.c)
 *
 */
static double readFilter(HOAMBDEC* p, int32_t i, int32_t j)
{

    double* readPoint; /* Generic pointer address */
    int32_t delay;
    /* Calculate the address of the index for this read */
    readPoint = p->currPos[j] - i;
    delay = p->ndelay;

    /* Wrap around for time-delay if necessary */
    if (readPoint < ((double*)p->delay[j].auxp) )
      readPoint += delay;
    else
      /* Wrap for time-advance if necessary */
      if (readPoint > ((double*)p->delay[j].auxp + (delay-1)) )
        readPoint -= delay;

    return *readPoint; /* Dereference read address for delayed value */
}


/* insertFilter -- delay-line update routine (band splitting)
 *
 * Inserts the passed value into the currPos and increments the
 * currPos pointer modulo the length of the delay line.
 * (adapted from  csound/Opcodes/hrtfopcodes.c)
 *
 */
static void insertFilter(HOAMBDEC* p, double val, int32_t j)
{

    int32_t delay;
    delay = p->ndelay;

    *p->currPos[j] = val;

    /* Update the currPos pointer and wrap modulo the delay length */
    if (((double*) (++p->currPos[j])) > ((double*)p->delay[j].auxp + (delay-1)) )
      p->currPos[j] -= delay;

}

/* process_nfc -- process NFC filter
 *
 * Process NFC filter.
 * This code was adapted from The Ambisonic Decoder Toolbox
 *
 */
static void process_nfc(CSOUND *csound, HOAMBDEC* p, int32_t signal_order, int32_t n, int32_t j, int32_t in_ix, int32_t sr,
                        int32_t ksmps)
{
    //char buffer[50];

    double d; // meters
    if ((int)*(p->r) == 0) {
      d = 1.0;
    } else {
      d = (double)*(p->r);
    }

    double c = 343.2; // m/s
    double omega = c/(d*sr);
    double gain = 1.0;
    MYFLT *in = p->in->data;
    //MYFLT *out = p->out->data;
    //  1  1

    if (signal_order == 1) {
      double b1 = omega/2.0;
      double g1 = 1.0 + b1;
      double d1 = 0.0 - (2.0 * b1) / g1;
      double g = gain/g1;

      double fTemp0 = (g * in[in_ix*ksmps+n]);
      double fTemp1 = (d1 * p->fRec0[j][1]);
      p->fRec2[j][0] = (fTemp0 + p->fRec2[j][1] + fTemp1);
      p->fRec0[j][0] = p->fRec2[j][0];
      double fRec1 = (fTemp1 + fTemp0);
      in[in_ix*ksmps+n] = (MYFLT) fRec1;
      p->fRec2[j][1] = p->fRec2[j][0];
      p->fRec0[j][1] = p->fRec0[j][0];
    }
    if (signal_order == 2) {
      double r1 = omega/2.0;
      double r2 = r1 * r1;

      // 1.000000000000000   3.00000000000000   3.00000000000000
      double b1 = 3.0 * r1;
      double b2 = 3.0 * r2;
      double g2 = 1.0 + b1 + b2;

      double d1 = 0.0 - (2.0 * b1 + 4.0 * b2) / g2;  // fixed
      double d2 = 0.0 - (4.0 * b2) / g2;
      double g = gain/g2;

      double fTemp0 = (g * in[in_ix*ksmps+n]);
      double fTemp1 = (d2 * p->fRec0[j][1]);
      double fTemp2 = (d1 * p->fRec3[j][1]);
      p->fRec5[j][0] = (fTemp0 + (fTemp1 + (p->fRec5[j][1] + fTemp2)));
      p->fRec3[j][0] = p->fRec5[j][0];
      float fRec4 = ((fTemp2 + fTemp1) + fTemp0);
      p->fRec2[j][0] = (p->fRec3[j][0] + p->fRec2[j][1]);
      p->fRec0[j][0] =  p->fRec2[j][0];
      float fRec1 = fRec4;
      in[in_ix*ksmps+n] = (MYFLT) fRec1;
      p->fRec5[j][1] = p->fRec5[j][0];
      p->fRec3[j][1] = p->fRec3[j][0];
      p->fRec2[j][1] = p->fRec2[j][0];
      p->fRec0[j][1] = p->fRec0[j][0];
    }

    if (signal_order == 3) {
      double r1 = omega/2.0;
      double r2 = r1 * r1;

      // 1.000000000000000   3.677814645373914   6.459432693483369
      double b1 = 3.677814645373914 * r1;
      double b2 = 6.459432693483369 * r2;
      double g2 = 1.0 + b1 + b2;
      double d1 = 0.0 - (2.0 * b1 + 4.0 * b2) / g2;  // fixed
      double d2 = 0.0 - (4.0 * b2) / g2;

      // 1.000000000000000   2.322185354626086
      double b3 = 2.322185354626086 * r1;
      double g3 = 1.0 + b3;
      double d3 = 0.0 - (2.0 * b3) / g3;

      double g = gain/(g3*g2);
      double fTemp0 = (d3 * p->fRec0[j][1]);
      double fTemp1 = (g * in[in_ix*ksmps+n]);
      double fTemp2 = (d2 * p->fRec3[j][1]);
      double fTemp3 = (d1 * p->fRec6[j][1]);
      p->fRec8[j][0] = (fTemp1 + (fTemp2 + (p->fRec8[j][1] + fTemp3)));
      p->fRec6[j][0] = p->fRec8[j][0];
      float fRec7 = ((fTemp3 + fTemp2) + fTemp1);
      p->fRec5[j][0] = (p->fRec6[j][0] + p->fRec5[j][1]);
      p->fRec3[j][0] = p->fRec5[j][0];
      float fRec4 = fRec7;
      p->fRec2[j][0] = (fTemp0 + (fRec4 + p->fRec2[j][1]));
      p->fRec0[j][0] = p->fRec2[j][0];
      float fRec1 = (fRec4 + fTemp0);
      in[in_ix*ksmps+n] = (MYFLT) fRec1;
      p->fRec8[j][1] = p->fRec8[j][0];
      p->fRec6[j][1] = p->fRec6[j][0];
      p->fRec5[j][1] = p->fRec5[j][0];
      p->fRec3[j][1] = p->fRec3[j][0];
      p->fRec2[j][1] = p->fRec2[j][0];
      p->fRec0[j][1] = p->fRec0[j][0];
    }

    if (signal_order == 4) {

      double r1 = omega/2.0;
      double r2 = r1 * r1;

      // 1.000000000000000   4.207578794359250  11.487800476871168
      double b1 =  4.207578794359250 * r1;
      double b2 = 11.487800476871168 * r2;
      double g2 = 1.0 + b1 + b2;
      double d1 = 0.0 - (2.0 * b1 + 4.0 * b2) / g2;  // fixed
      double d2 = 0.0 - (4.0 * b2) / g2;

      // 1.000000000000000   5.792421205640748   9.140130890277934
      double b3 = 5.792421205640748 * r1;
      double b4 = 9.140130890277934 * r2;
      double g3 = 1.0 + b3 + b4;
      double d3 = 0.0 - (2.0 * b3 + 4.0 * b4) / g3;  // fixed
      double d4 = 0.0 - (4.0 * b4) / g3;

      double g = gain/(g3*g2);

      double fTemp0 = (d4 * p->fRec0[j][1]);
      double fTemp1 = (d3 * p->fRec3[j][1]);
      double fTemp2 = (d2 * p->fRec6[j][1]);
      double fTemp3 = (d1 * p->fRec9[j][1]);
      double fTemp4 = (g * in[in_ix*ksmps+n]);
      p->fRec11[j][0] = ((fTemp2 + (p->fRec11[j][1] + fTemp3)) + fTemp4);
      p->fRec9[j][0] = p->fRec11[j][0];
      double fRec10 = ((fTemp3 + fTemp2) + fTemp4);
      p->fRec8[j][0] = (p->fRec9[j][0] + p->fRec8[j][1]);
      p->fRec6[j][0] = p->fRec8[j][0];
      double fRec7 = fRec10;
      p->fRec5[j][0] = (fTemp0 + (fTemp1 + (fRec7 + p->fRec5[j][1])));
      p->fRec3[j][0] = p->fRec5[j][0];
      double fRec4 = (fTemp0 + (fRec7 + fTemp1));
      p->fRec2[j][0] = (p->fRec3[j][0] + p->fRec2[j][1]);
      p->fRec0[j][0] = p->fRec2[j][0];
      double fRec1 = fRec4;
      in[in_ix*ksmps+n] = (MYFLT) fRec1;
      p->fRec11[j][1] = p->fRec11[j][0];
      p->fRec9[j][1] = p->fRec9[j][0];
      p->fRec8[j][1] = p->fRec8[j][0];
      p->fRec6[j][1] = p->fRec6[j][0];
      p->fRec5[j][1] = p->fRec5[j][0];
      p->fRec3[j][1] = p->fRec3[j][0];
      p->fRec2[j][1] = p->fRec2[j][0];
      p->fRec0[j][1] = p->fRec0[j][0];

    }

    if (signal_order == 5) {

      double r1 = omega/2.0;
      double r2 = r1 * r1;

      // 1.000000000000000   4.649348606363304  18.156315313452325
      double b1 =  4.649348606363304 * r1;
      double b2 = 18.156315313452325 * r2;
      double g2 = 1.0 + b1 + b2;
      double d1 = 0.0 - (2.0 * b1 + 4.0 * b2) / g2;  // fixed
      double d2 = 0.0 - (4.0 * b2) / g2;

      // 1.000000000000000   6.703912798306966  14.272480513279568
      double b3 =  6.703912798306966 * r1;
      double b4 = 14.272480513279568 * r2;
      double g3 = 1.0 + b3 + b4;
      double d3 = 0.0 - (2.0 * b3 + 4 * b4) / g3;  // fixed
      double d4 = 0.0 - (4.0 * b4) / g3;

      // 1.000000000000000   3.646738595329718
      double b5 = 3.646738595329718 * r1;
      double g4 = 1.0 + b5;
      double d5 = 0.0 - (2.0 * b5) / g4;

      double g = gain/(g4*g3*g2);

      double fTemp0 = (d5 * p->fRec0[j][1]);
      double fTemp1 = (d4 * p->fRec3[j][1]);
      double fTemp2 = (d3 * p->fRec6[j][1]);
      double fTemp3 = (d2 * p->fRec9[j][1]);
      double fTemp4 = (d1 * p->fRec12[j][1]);
      double fTemp5 = (g * in[in_ix*ksmps+n]);
      p->fRec14[j][0] = ((fTemp3 + (p->fRec14[j][1] + fTemp4)) + fTemp5);
      p->fRec12[j][0] = p->fRec14[j][0];
      double fRec13 = ((fTemp4 + fTemp3) + fTemp5);
      p->fRec11[j][0] = (p->fRec12[j][0] + p->fRec11[j][1]);
      p->fRec9[j][0] = p->fRec11[j][0];
      double fRec10 = fRec13;
      p->fRec8[j][0] = (fTemp1 + (fTemp2 + (fRec10 + p->fRec8[j][1])));
      p->fRec6[j][0] = p->fRec8[j][0];
      double fRec7 = (fTemp1 + (fRec10 + fTemp2));
      p->fRec5[j][0] = (p->fRec6[j][0] + p->fRec5[j][1]);
      p->fRec3[j][0] = p->fRec5[j][0];
      double fRec4 = fRec7;
      p->fRec2[j][0] = (fTemp0 + (fRec4 + p->fRec2[j][1]));
      p->fRec0[j][0] = p->fRec2[j][0];
      double fRec1 = (fRec4 + fTemp0);
      in[in_ix*ksmps+n] = (MYFLT) fRec1;
      p->fRec14[j][1] = p->fRec14[j][0];
      p->fRec12[j][1] = p->fRec12[j][0];
      p->fRec11[j][1] = p->fRec11[j][0];
      p->fRec9[j][1] = p->fRec9[j][0];
      p->fRec8[j][1] = p->fRec8[j][0];
      p->fRec6[j][1] = p->fRec6[j][0];
      p->fRec5[j][1] = p->fRec5[j][0];
      p->fRec3[j][1] = p->fRec3[j][0];
      p->fRec2[j][1] = p->fRec2[j][0];
      p->fRec0[j][1] = p->fRec0[j][0];

    }

}

#define S(x)    sizeof(x)
extern "C" {
static OENTRY bformdec2_localops[] = {
  { (char*) "bformdec2.A", S(HOAMBDEC), 0, (char*) "a[]", (char*) "ia[]ooooNN",
    (SUBR)ihoambdec, (SUBR)ahoambdec },
};

LINKAGE_BUILTIN(bformdec2_localops)
}
