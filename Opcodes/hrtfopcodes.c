/*
    hrtfopcodes.c: new HRTF opcodes

    (c) Brian Carty, 2008

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
#include "csdl.h"
#include <math.h>
/*definitions*/
#define minelev -40             /*from mit*/
#define elevincrement 10        /*from mit*/

#define maxdeltime 0.0011       /*max delay for min phase: a time value:
                                  important to include offset...multiply by
                                  sr to get no of samples for memory allocation*/

/*additional definitions for woodworth models*/
#define c 34400.0

/*hrtf data sets were analysed for low frequency phase values, as it
  is the important part of the spectrum for phase based localisation
  cues. The values below were extracted and are used to scale the
  functional phase spectrum.*/
static const float nonlinitd[5] = {1.535133f,1.347668f,1.111214f,1.071324f,1.0f};
static const float nonlinitd48k[5] = {1.515308f,1.268052f,1.086033f,1.086159f,1.0f};
static const float nonlinitd96k[5] = {1.515846f,1.268239f,1.086041f,1.086115f,1.0f};

/*number of measurements per elev: mit data const:read only, static:exists
  for whoe process...*/
static const int elevationarray[14] = {56, 60, 72, 72, 72, 72, 72,
                                       60, 56, 45, 36, 24, 12, 1 };

/*assumed mit hrtf data will be used here. Otherwise delay data would
  need to be extracted and replaced here...*/
static const float minphasedels[368] = {
  0.000000f,0.000045f,0.000091f,0.000136f,0.000159f,0.000204f,
  0.000249f,0.000272f,0.000295f,0.000317f,0.000363f,0.000385f,
  0.000272f,0.000408f,0.000454f,0.000454f,0.000408f,0.000385f,
  0.000363f,0.000317f,0.000295f,0.000295f,0.000249f,0.000204f,
  0.000159f,0.000136f,0.000091f,0.000045f,0.000000f,0.000000f,
  0.000045f,0.000091f,0.000136f,0.000181f,0.000227f,0.000249f,
  0.000272f,0.000317f,0.000363f,0.000385f,0.000454f,0.000476f,
  0.000454f,0.000522f,0.000499f,0.000499f,0.000476f,0.000454f,
  0.000408f,0.000408f,0.000385f,0.000340f,0.000295f,0.000272f,
  0.000227f,0.000181f,0.000136f,0.000091f,0.000045f,0.000000f,
  0.000000f,0.000045f,0.000091f,0.000113f,0.000159f,0.000204f,
  0.000227f,0.000272f,0.000317f,0.000317f,0.000363f,0.000408f,
  0.000363f,0.000522f,0.000476f,0.000499f,0.000590f,0.000567f,
  0.000567f,0.000544f,0.000522f,0.000499f,0.000476f,0.000454f,
  0.000431f,0.000408f,0.000385f,0.000363f,0.000317f,0.000295f,
  0.000249f,0.000204f,0.000181f,0.000136f,0.000091f,0.000045f,
  0.000000f,0.000000f,0.000045f,0.000091f,0.000113f,0.000159f,
  0.000204f,0.000249f,0.000295f,0.000317f,0.000363f,0.000340f,
  0.000385f,0.000431f,0.000476f,0.000522f,0.000544f,0.000612f,
  0.000658f,0.000658f,0.000635f,0.000658f,0.000522f,0.000499f,
  0.000476f,0.000454f,0.000408f,0.000385f,0.000363f,0.000340f,
  0.000295f,0.000272f,0.000227f,0.000181f,0.000136f,0.000091f,
  0.000045f,0.000000f,0.000000f,0.000045f,0.000091f,0.000136f,
  0.000159f,0.000204f,0.000249f,0.000295f,0.000340f,0.000385f,
  0.000431f,0.000476f,0.000522f,0.000567f,0.000522f,0.000567f,
  0.000567f,0.000635f,0.000703f,0.000748f,0.000748f,0.000726f,
  0.000703f,0.000658f,0.000454f,0.000431f,0.000385f,0.000363f,
  0.000317f,0.000295f,0.000272f,0.000227f,0.000181f,0.000136f,
  0.000091f,0.000045f,0.000000f,0.000000f,0.000045f,0.000091f,
  0.000113f,0.000159f,0.000204f,0.000249f,0.000295f,0.000340f,
  0.000385f,0.000408f,0.000454f,0.000499f,0.000544f,0.000522f,
  0.000590f,0.000590f,0.000635f,0.000658f,0.000680f,0.000658f,
  0.000544f,0.000590f,0.000567f,0.000454f,0.000431f,0.000385f,
  0.000363f,0.000317f,0.000272f,0.000272f,0.000227f,0.000181f,
  0.000136f,0.000091f,0.000045f,0.000000f,0.000000f,0.000045f,
  0.000068f,0.000113f,0.000159f,0.000204f,0.000227f,0.000272f,
  0.000317f,0.000340f,0.000385f,0.000431f,0.000454f,0.000499f,
  0.000499f,0.000544f,0.000567f,0.000590f,0.000590f,0.000590f,
  0.000590f,0.000567f,0.000567f,0.000476f,0.000454f,0.000408f,
  0.000385f,0.000340f,0.000340f,0.000295f,0.000249f,0.000204f,
  0.000159f,0.000136f,0.000091f,0.000045f,0.000000f,0.000000f,
  0.000045f,0.000091f,0.000113f,0.000159f,0.000204f,0.000249f,
  0.000295f,0.000340f,0.000363f,0.000385f,0.000431f,0.000454f,
  0.000499f,0.000522f,0.000522f,0.000522f,0.000499f,0.000476f,
  0.000454f,0.000431f,0.000385f,0.000340f,0.000317f,0.000272f,
  0.000227f,0.000181f,0.000136f,0.000091f,0.000045f,0.000000f,
  0.000000f,0.000045f,0.000091f,0.000136f,0.000159f,0.000204f,
  0.000227f,0.000249f,0.000295f,0.000340f,0.000363f,0.000385f,
  0.000408f,0.000431f,0.000431f,0.000431f,0.000431f,0.000408f,
  0.000385f,0.000363f,0.000317f,0.000317f,0.000272f,0.000227f,
  0.000181f,0.000136f,0.000091f,0.000045f,0.000000f,0.000000f,
  0.000045f,0.000091f,0.000136f,0.000181f,0.000204f,0.000227f,
  0.000272f,0.000295f,0.000317f,0.000340f,0.000340f,0.000363f,
  0.000363f,0.000340f,0.000317f,0.000295f,0.000249f,0.000204f,
  0.000159f,0.000113f,0.000068f,0.000023f,0.000000f,0.000045f,
  0.000068f,0.000113f,0.000159f,0.000181f,0.000204f,0.000227f,
  0.000249f,0.000249f,0.000249f,0.000227f,0.000227f,0.000181f,
  0.000159f,0.000113f,0.000091f,0.000045f,0.000000f,0.000000f,
  0.000045f,0.000091f,0.000136f,0.000159f,0.000181f,0.000181f,
  0.000181f,0.000159f,0.000136f,0.000091f,0.000045f,0.000000f,
  0.000000f,0.000045f,0.000068f,0.000091f,0.000068f,0.000045f,
  0.000000f,0.000000f};


#ifdef WORDS_BIGENDIAN
static int swap4bytes(CSOUND* csound, MEMFIL* mfp)
{
    char c1, c2, c3, c4;
    char *p = mfp->beginp;
    int  size = mfp->length;

    while (size >= 4) {
      c1 = p[0]; c2 = p[1]; c3 = p[2]; c4 = p[3];
      p[0] = c4; p[1] = c3; p[2] = c2; p[3] = c1;
      size -= 4; p +=4;
    }

    return OK;
}
#else
static int (*swap4bytes)(CSOUND*, MEMFIL*) = NULL;
#endif

/*Csound hrtf magnitude interpolation, phase truncation object: Jan 08*/

/*aleft,aright hrtfmove asrc, kaz, kel, ifilel, ifiler [, imode =0,
  ifade =8, sr = 44100]...*/
/*imode: minphase/phase truncation, ifade: no of buffers per fade for
  phase trunc., sr can be 44.1/48/96k*/

typedef struct {
  OPDS  h;
  MYFLT *outsigl, *outsigr;
  MYFLT *in, *kangle, *kelev, *ifilel, *ifiler,
         *omode, *ofade, *osr;   /* outputs and inputs*/

  MEMFIL *fpl,*fpr,*fpdel;                              /* file pointers*/
  float *fpbeginl,*fpbeginr;

  int IMPLENGTH, complexIMPLENGTH, overlapsize,
      complexfftbuff;         /*see definitions in INIT*/

  MYFLT sr;

  int oldelevindex, oldangleindex;      /*old indices for checking if
                                          changes occur in trajectory.*/

  int counter;

  int cross,l,initialfade;              /*initialfade used to avoid fade
                                          in of data...if not,'old' data
                                          faded out with zero hrtf,'new'
                                          data faded in.*/

  int fadebuffer;               /*user defined buffer size for fades.*/

  int minphase,phasetrunc;              /*flags for process type*/

  AUXCH hrtflpad,hrtfrpad;              /*hrtf data padded*/
  AUXCH oldhrtflpad,oldhrtfrpad;        /*old data for fades*/
  AUXCH insig, outl, outr, outlold, outrold;    /*in and output buffers*/

  /*memory local to perform method*/
  AUXCH complexinsig;                   /*insig fft*/
  AUXCH hrtflfloat, hrtfrfloat;         /*hrtf buffers (rectangular complex form)*/
  AUXCH outspecl, outspecr, outspecoldl, outspecoldr;   /*spectral data*/

  AUXCH overlapl, overlapr;             /*overlap data*/
  AUXCH overlapoldl, overlapoldr;       /*old overlap data for longer crossfades*/

  AUXCH lowl1,lowr1,lowl2,lowr2,
        highl1,highr1,highl2,highr2;    /*interpolation buffers*/
  AUXCH currentphasel, currentphaser;   /*current phase buffers*/

  AUXCH logmagl,logmagr,xhatwinl,
        xhatwinr,expxhatwinl,expxhatwinr;      /*min phase buffers*/
  AUXCH win;            /*min phase window: a static buffer*/

  /*delay stuff*/
  AUXCH delmeml, delmemr;
  int ptl,ptr;

  int counts;

} hrtfmove;

static int hrtfmove_init(CSOUND *csound, hrtfmove *p)
{
    MEMFIL *fpl=NULL,*fpr=NULL; /*left and right data files: spectral mag,
                                  phase format.*/
    int i;                        /*for looping*/
    char filel[MAXNAME],filer[MAXNAME];

    int mode = (int)*p->omode;
    int fade = (int)*p->ofade;
    MYFLT sr = *p->osr;

    MYFLT *win;

    int IMPLENGTH = 0;                /*time domain impulse length */
    int complexIMPLENGTH = 0;         /*freq domain impulse length*/
    int overlapsize = 0;              /*overlap add convolution*/
    int complexfftbuff = 0;           /*complex fft used(min phase needs it)*/


    /* csound->Message(csound,
                       Str("\n\nHrtf Based Binarual Spatialisation\n\n")); */

    if (mode==1) p->minphase=1;    /*flag for process type: default phase trunc*/
    else p->phasetrunc=1;

    if (fade<1||fade>24)           /*fade length: default 8, max 24, min 1 */
      fade=8;

    if (sr!=FL(44100.0) && sr!=FL(48000.0) && sr!=FL(96000.0))
      sr = FL(44100.0);
    p->sr = sr;

    if (UNLIKELY(csound->esr != sr))
      csound->Warning(csound,
                      Str("\nOrchestra sampling rate is not"
                          " compatible with HRTF data files\nShould be %.0f,"
                          " see Csound help for object\n\n"), sr);

    strcpy(filel, (char*) p->ifilel); /*copy in string name...*/
    strcpy(filer, (char*) p->ifiler);

    if (sr == FL(44100.0)) {               /*setup as per sr*/
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      /*added ldmemfile2: reading floats without a header!*/
      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }
      /* Byte swap */
      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
      /* Byte swap */
    }

    else if (sr == FL(48000.0)) {
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    else if (sr == FL(96000.0)) {
      IMPLENGTH = 256;
      complexIMPLENGTH = 512;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }
    else {

      return
        csound->InitError(csound,
                          Str("\n\n\n Sampling rate not supported, exiting\n\n"));
    }

    p->IMPLENGTH = IMPLENGTH;
    p->complexIMPLENGTH = complexIMPLENGTH;
    p->overlapsize = overlapsize;
    p->complexfftbuff = complexfftbuff;

    p->fadebuffer = (int)fade*IMPLENGTH; /*the amount of buffers to fade over.*/

    /*file handles*/
    if (fpl && fpr) {
      p->fpl = fpl;
      p->fpr = fpr;
      p->fpbeginl = (float *) fpl->beginp;
      p->fpbeginr = (float *) fpr->beginp;
    }

    /*common buffers (used by both min phase and phasetrunc)*/
    csound->AuxAlloc(csound, IMPLENGTH*sizeof(MYFLT), &p->insig);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outr);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->hrtflpad);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->hrtfrpad);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p-> complexinsig);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtflfloat);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtfrfloat);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecl);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecr);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapl);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapr);

    /*interpolation values*/
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->currentphasel);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->currentphaser);

    p->counter = 0;               /*initialize counter*/

    /*phase truncation buffers and variables*/
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->oldhrtflpad);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->oldhrtfrpad);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outlold);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outrold);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecoldl);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecoldr);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapoldl);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapoldr);

    /* initialize counters and indices*/
    p->cross = 0;
    p->l = 0;
    p->initialfade = 0;

    /*need to be a value that is not possible for first check to avoid
      phase not being read.*/
    p->oldelevindex = -1;
    p->oldangleindex = -1;

    /*buffer declaration for min phase calculations*/
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->logmagl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->logmagr);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->xhatwinl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->xhatwinr);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->expxhatwinl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->expxhatwinr);

    /*delay buffers*/
    csound->AuxAlloc(csound, (int)(sr*maxdeltime)*sizeof(MYFLT), &p->delmeml);
    csound->AuxAlloc(csound, (int)(sr*maxdeltime)*sizeof(MYFLT), &p->delmemr);

    csound->AuxAlloc(csound, IMPLENGTH*sizeof(MYFLT), &p->win);

    win = (MYFLT *)p->win.auxp;

    /*min phase win defined for implength point impulse!*/
    win[0]=1;
    for (i=1;i<(IMPLENGTH/2);i++) win[i]=2;
    win[(IMPLENGTH/2)]=1;
    for (i=((IMPLENGTH/2)+1);i<IMPLENGTH;i++) win[i]=0;

    p->ptl=0;
    p->ptr=0;

    p->counts = 0;

    return OK;
}


static int hrtfmove_process(CSOUND *csound, hrtfmove *p)
{
    MYFLT *in = p->in;                    /*local pointers to p*/
    MYFLT *outsigl = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /*common buffers and variables*/
    MYFLT *insig = (MYFLT *)p->insig.auxp;
    MYFLT *outl = (MYFLT *)p->outl.auxp;
    MYFLT *outr = (MYFLT *)p->outr.auxp;

    MYFLT *hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    MYFLT *hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    MYFLT *hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT *overlapl = (MYFLT *)p->overlapl.auxp;
    MYFLT *overlapr = (MYFLT *)p->overlapr.auxp;

    MYFLT elev = *p->kelev;
    MYFLT angle = *p->kangle;

    int counter = p->counter;
    int n;

    float *fpindexl;      /* pointers into HRTF files: floating point data
                             (even in 64 bit csound)*/
    float *fpindexr;

    int i,j,elevindex, angleindex, switchchannels=0, skip=0;

    int minphase = p->minphase;
    int phasetrunc = p->phasetrunc;

    MYFLT sr = p->sr;

    int IMPLENGTH = p->IMPLENGTH;
    int complexIMPLENGTH = p->complexIMPLENGTH;
    int overlapsize = p->overlapsize;
    int complexfftbuff = p->complexfftbuff;

    /*interpolation values*/
    MYFLT *lowl1 = (MYFLT *)p->lowl1.auxp;
    MYFLT *lowr1 = (MYFLT *)p->lowr1.auxp;
    MYFLT *lowl2 = (MYFLT *)p->lowl2.auxp;
    MYFLT *lowr2 = (MYFLT *)p->lowr2.auxp;
    MYFLT *highl1 = (MYFLT *)p->highl1.auxp;
    MYFLT *highr1 = (MYFLT *)p->highr1.auxp;
    MYFLT *highl2 = (MYFLT *)p->highl2.auxp;
    MYFLT *highr2 = (MYFLT *)p->highr2.auxp;
    MYFLT *currentphasel = (MYFLT *)p->currentphasel.auxp;
    MYFLT *currentphaser = (MYFLT *)p->currentphaser.auxp;

    /*local interpolation values*/
    MYFLT elevindexlowper,elevindexhighper,angleindex1per,
      angleindex2per,angleindex3per,angleindex4per;
    int elevindexlow,elevindexhigh,angleindex1,angleindex2,
      angleindex3,angleindex4;
    MYFLT magl,magr,phasel,phaser, magllow, magrlow, maglhigh, magrhigh;

    /*phase truncation buffers and variables*/
    MYFLT *oldhrtflpad = (MYFLT *)p->oldhrtflpad.auxp;
    MYFLT *oldhrtfrpad = (MYFLT *)p->oldhrtfrpad.auxp;
    MYFLT *outlold = (MYFLT *)p->outlold.auxp;
    MYFLT *outrold = (MYFLT *)p->outrold.auxp;
    MYFLT *outspecoldl = (MYFLT *)p->outspecoldl.auxp;
    MYFLT *outspecoldr = (MYFLT *)p->outspecoldr.auxp;
    MYFLT *overlapoldl = (MYFLT *)p->overlapoldl.auxp;
    MYFLT *overlapoldr = (MYFLT *)p->overlapoldr.auxp;

    int oldelevindex = p ->oldelevindex;
    int oldangleindex = p ->oldangleindex;

    int cross = p ->cross;
    int l = p->l;
    int initialfade = p ->initialfade;

    int crossfade = 0;
    int crossout = 0;

    int fadebuffer = p->fadebuffer;

    /*minimum phase buffers*/
    MYFLT *logmagl = (MYFLT *)p->logmagl.auxp;
    MYFLT *logmagr = (MYFLT *)p->logmagr.auxp;
    MYFLT *xhatwinl = (MYFLT *)p->xhatwinl.auxp;
    MYFLT *xhatwinr = (MYFLT *)p->xhatwinr.auxp;
    MYFLT *expxhatwinl = (MYFLT *)p->expxhatwinl.auxp;
    MYFLT *expxhatwinr = (MYFLT *)p->expxhatwinr.auxp;

    MYFLT *win = (MYFLT *)p->win.auxp;     /*min phase window*/

    /*min phase delay variables*/
    MYFLT *delmeml = (MYFLT *)p->delmeml.auxp;
    MYFLT *delmemr = (MYFLT *)p->delmemr.auxp;
    MYFLT delaylow1,delaylow2,delayhigh1,delayhigh2,delaylow,delayhigh,delayfloat;
    int ptl = p->ptl;
    int ptr = p->ptr;

    int mdtl=0,mdtr=0;
    MYFLT outvdl,outvdr,rpl,rpr,vdtl,vdtr,fracl,fracr;

    /*start indices at correct value (start of file)/ zero indices.*/
    fpindexl = (float *) p->fpbeginl;
    fpindexr = (float *) p->fpbeginr;

    n=csound->ksmps;

    for (j=0;j<n;j++) {
      insig[counter] = in[j];                   /*ins and outs*/

      outsigl[j]=outl[counter];
      outsigr[j]=outr[counter];

      counter++;

      p->counts++;

      if (phasetrunc) {
        if (initialfade<(IMPLENGTH+2))
          initialfade++;
      }  /*used to ensure fade does not happen on first run*/

      if (counter == IMPLENGTH) {         /*process a block*/
        if (elev < -40) elev = -40;            /*within legal MIT range*/
        if (elev > 90) elev = 90;
        elevindex = (int)floor((float)((float)(elev-minelev)/elevincrement)+0.5f);

        while (UNLIKELY(angle<FL(0.0))) angle+=360;
        while (UNLIKELY(angle>FL(360))) angle-=360; /*mod 360*/
        if (angle>180) {
          angle=360-angle;
          switchchannels=1; /*true for later function: data is symmetrical*/
        }

        /*read using an index system based on number of points
          measured per elevation at mit*/
        angleindex =    /*angle/increment+0.5*/
          (int)floor(angle/(360/(MYFLT)elevationarray[elevindex])+0.5);
        if (angleindex>=((int)(elevationarray[elevindex]/2)+1))
          /*last point in current elevation*/
          angleindex=(int)(elevationarray[elevindex]/2);
        /* two nearest elev indices*/
        elevindexlow = (int)((elev-minelev)/elevincrement);
        if (elevindexlow<13) elevindexhigh = elevindexlow+1;
        else elevindexhigh = elevindexlow;            /* highest index reached*/

        /* get percentage value for interpolation*/
        elevindexlowper = (1.0f - (((float)(elev-minelev)/elevincrement)
                                   -(float)elevindexlow));
        elevindexhighper = 1.0f - elevindexlowper;

        /* 4 closest angle indices, 2 low and 2 high*/
        angleindex1 = (int)(angle/(360/(float)elevationarray[elevindexlow]));

        angleindex2 = angleindex1 + 1;
        if (angleindex2>=((int)(elevationarray[elevindexlow]/2)+1))
          angleindex2=(int)(elevationarray[elevindexlow]/2);

        angleindex3 = (int)(angle/(360/(float)elevationarray[elevindexhigh]));

        angleindex4 = angleindex3 + 1;
        if (angleindex4>=((int)(elevationarray[elevindexhigh]/2)+1))
          angleindex4=(int)(elevationarray[elevindexhigh]/2);

        /* angle percentages for interp*/
        angleindex1per =
          1.0f-(float)((angle/(360.0/(float)elevationarray[elevindexlow])-
                        angleindex1));
        angleindex2per = 1.0f-angleindex1per;
        angleindex3per =
          1.0f - (float)((angle/(360.0/(float)elevationarray[elevindexhigh])-
                          angleindex3));
        angleindex4per = 1.0f-angleindex3per;

        if (phasetrunc) {
          if (angleindex!=oldangleindex || elevindex!=oldelevindex) {
            /*store last point and turn crossfade on, provided
              that initialfade value indicates first block
              processed!*/
            /*(otherwise,there will be a fade in at the start as
              old hrtf will be filled with zeros and used).*/
            if (initialfade>IMPLENGTH) {
              /*post warning if fades ovelap*/
              if (UNLIKELY(cross))
                csound->Warning(csound,
                                Str("fades are overlapping: "
                                    "this could lead to noise: reduce "
                                    "fade size or change trajectory\n\n"));
              /*reset l, use as index to fade*/
              l=0;
              crossfade=1;
              /*store old data*/
              for (i=0;i<complexfftbuff;i++) {
                oldhrtflpad[i] = hrtflpad[i];
                oldhrtfrpad[i] = hrtfrpad[i];
              }
            }

            /*store point for current phase as trajectory comes
              closer to a new index  */
            if (elevindex == 0);
            else for (i=0; i<elevindex; i++)
                   skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

            if (angleindex == 0);
            else for (i=0; i<angleindex; i++)
                   skip += (2*IMPLENGTH);

            if (switchchannels) {
              for (i=0;i<complexIMPLENGTH;i++){
                currentphasel[i]=fpindexr[skip+i];
                currentphaser[i]=fpindexl[skip+i];
              }
            }
            else {
              for (i=0;i<complexIMPLENGTH;i++) {
                currentphasel[i]=fpindexl[skip+i];
                currentphaser[i]=fpindexr[skip+i];
              }
            }
          }
        }

        /*read 4 nearest points for interpolation*/
        /*point 1*/
        skip = 0;
        if (elevindexlow == 0);
        else for (i=0; i<elevindexlow; i++)
               /*skip * 2 as data is in complex mag,phase format*/
               skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex1 == 0);
        else for (i=0; i<angleindex1; i++)
               skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl1[i]=fpindexr[skip+i];
            lowr1[i]=fpindexl[skip+i];
          }
        }
        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl1[i]=fpindexl[skip+i];
            lowr1[i]=fpindexr[skip+i];
          }
        }

        /*point 2*/
        skip = 0;
        if (elevindexlow == 0);
        else for (i=0; i<elevindexlow; i++)
               skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex2 == 0);
        else for (i=0; i<angleindex2; i++)
               skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl2[i]=fpindexr[skip+i];
            lowr2[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl2[i]=fpindexl[skip+i];
            lowr2[i]=fpindexr[skip+i];
          }
        }

        /*point 3*/
        skip = 0;
        if (elevindexhigh == 0);
        else for (i=0; i<elevindexhigh; i++)
               skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex3 == 0);
        else for (i=0; i<angleindex3; i++)
               skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl1[i]=fpindexr[skip+i];
            highr1[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl1[i]=fpindexl[skip+i];
            highr1[i]=fpindexr[skip+i];
          }
        }

        /*point 4*/
        skip = 0;
        if (elevindexhigh == 0);
        else for (i=0; i<elevindexhigh; i++)
               skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex4 == 0);
        else for (i=0; i<angleindex4; i++)
               skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl2[i]=fpindexr[skip+i];
            highr2[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl2[i]=fpindexl[skip+i];
            highr2[i]=fpindexr[skip+i];
          }
        }

        /* magnitude interpolation*/
        for (i=0; i < complexIMPLENGTH; i+=2) {
          /* interpolate HIGH AND LOW MAGS*/
          magllow = lowl1[i]+(lowl2[i]-lowl1[i])*angleindex2per;
          maglhigh = highl1[i]+(highl2[i]-highl1[i])*angleindex4per;

          magrlow = lowr1[i]+(lowr2[i]-lowr1[i])*angleindex2per;
          magrhigh = highr1[i]+(highr2[i]-highr1[i])*angleindex4per;

          /* interpolate high and low results*/
          magl = magllow+(maglhigh-magllow)*elevindexhighper;
          magr = magrlow+(magrhigh-magrlow)*elevindexhighper;

          if (phasetrunc) {       /*use current phase, back to rectangular*/
            phasel = currentphasel[i+1];
            phaser = currentphaser[i+1];

            /* polar to rectangular*/
            hrtflfloat[i] = magl*COS(phasel);
            hrtflfloat[i+1] = magl*SIN(phasel);

            hrtfrfloat[i] = magr*COS(phaser);
            hrtfrfloat[i+1] = magr*SIN(phaser);
          }

          if (minphase) {
            /*store log magnitudes, 0 phases for ifft, do not allow log(0.0)*/
            logmagl[i]= LOG(magl==FL(0.0)?FL(0.00000001):magl);
            logmagr[i]= LOG(magr==FL(0.0)?FL(0.00000001):magr);

            logmagl[i+1] = FL(0.0);
            logmagr[i+1] = FL(0.0);
          }
        }

        if (minphase) {
          /*ifft!...see Oppehneim and Schafer for min phase process...
            based on real cepstrum method*/
          csound->InverseComplexFFT(csound, logmagl, IMPLENGTH);
          csound->InverseComplexFFT(csound, logmagr, IMPLENGTH);

          /*window, note no need to scale on csound iffts...*/
          for (i=0;i<complexIMPLENGTH;i+=2)            {
            xhatwinl[i] = (logmagl[i]) * win[i/2];
            xhatwinr[i] = (logmagr[i]) * win[i/2];
            xhatwinl[i+1] = FL(0.0);
            xhatwinr[i+1] = FL(0.0);
          }

          /*fft*/
          csound->ComplexFFT(csound, xhatwinl, IMPLENGTH);
          csound->ComplexFFT(csound, xhatwinr, IMPLENGTH);

          /*exponential of complex result*/
          for (i=0;i<256;i+=2) {
            expxhatwinl[i] = EXP(xhatwinl[i])*COS(xhatwinl[i+1]);
            expxhatwinl[i+1] = EXP(xhatwinl[i])*SIN(xhatwinl[i+1]);
            expxhatwinr[i] = EXP(xhatwinr[i])*COS(xhatwinr[i+1]);
            expxhatwinr[i+1] = EXP(xhatwinr[i])*SIN(xhatwinr[i+1]);
          }

          /*ifft for output buffers*/
          csound->InverseComplexFFT(csound, expxhatwinl, IMPLENGTH);
          csound->InverseComplexFFT(csound, expxhatwinr, IMPLENGTH);

          /*real parts of ifft for output, scale. */
          for (i= 0; i < complexIMPLENGTH; i+=2) {
            hrtflpad[i] = (expxhatwinl[i]);
            hrtfrpad[i] = (expxhatwinr[i]);
            hrtflpad[i+1] = FL(0.0);
            hrtfrpad[i+1] = FL(0.0);
          }
        }

        if (phasetrunc)        /*use current phase and interped mag directly*/
          {
            /*ifft*/
            csound->InverseComplexFFT(csound, hrtflfloat, IMPLENGTH);
            csound->InverseComplexFFT(csound, hrtfrfloat, IMPLENGTH);

            for (i=0; i<complexIMPLENGTH; i++) {
              /* scale and pad buffers with zeros to fftbuff*/
              hrtflpad[i] = hrtflfloat[i];
              hrtfrpad[i] = hrtfrfloat[i];
            }
          }

        /*zero pad impulse*/
        for (i=complexIMPLENGTH;i<complexfftbuff;i++)
          {
            hrtflpad[i]=FL(0.0);
            hrtfrpad[i]=FL(0.0);
          }

        /*back to freq domain*/
        csound->ComplexFFT(csound, hrtflpad, complexIMPLENGTH);
        csound->ComplexFFT(csound, hrtfrpad, complexIMPLENGTH);

        /* look after overlap add stuff*/
        for (i = 0; i < overlapsize ; i++) {
          overlapl[i] = outl[i+IMPLENGTH];
          overlapr[i] = outr[i+IMPLENGTH];

          if (phasetrunc)            /*look after fade stuff*/
            {
              if (crossfade)
                {
                  overlapoldl[i] = outl[i+IMPLENGTH];
                  overlapoldr[i] = outr[i+IMPLENGTH];
                }
              /* overlap will be previous fading out signal*/
              if (cross)
                {
                  overlapoldl[i] = outlold[i+IMPLENGTH];
                  overlapoldr[i] = outrold[i+IMPLENGTH];
                }
            }
        }

        /* insert insig for complex real,im fft, zero pad*/
        for (i = 0; i <  IMPLENGTH; i++)
          {
            complexinsig[2*i] = insig[i];
            complexinsig[(2*i)+1] = FL(0.0);
          }

        for (i = complexIMPLENGTH; i <  complexfftbuff; i++)
          complexinsig[i] = FL(0.0);

        csound->ComplexFFT(csound, complexinsig, complexIMPLENGTH);

        /* complex multiplication*/
        for (i = 0; i < complexfftbuff; i+=2) {
          outspecl[i] =
            complexinsig[i]*hrtflpad[i] - complexinsig[i+1]*hrtflpad[i+1];
          outspecr[i] =
            complexinsig[i]*hrtfrpad[i] - complexinsig[i+1]*hrtfrpad[i+1];
          outspecl[i+1] =
            complexinsig[i]*hrtflpad[i+1] + complexinsig[i+1]*hrtflpad[i];
          outspecr[i+1] =
            complexinsig[i]*hrtfrpad[i+1] + complexinsig[i+1]*hrtfrpad[i];
        }

        /* convolution is the inverse FFT of above result */
        csound->InverseComplexFFT(csound, outspecl, complexIMPLENGTH);
        csound->InverseComplexFFT(csound, outspecr, complexIMPLENGTH);

        if (phasetrunc) {
          /*real values, scaled (by a little more than usual to ensure
            no clipping) sr related?*/
          for (i = 0; i < complexIMPLENGTH; i++)
            {
              outl[i] = outspecl[2*i]/(sr/FL(38000.0));
              outr[i] = outspecr[2*i]/(sr/FL(38000.0));
            }
        }

        if (minphase) {
          /*real values*/

          /*scaling relative to sr not necessary for min phase?...just
            scaling by a little > 1 to avoid any possible overlap add clips*/

          for (i = 0; i < complexIMPLENGTH; i++)
            {
              outl[i] = outspecl[2*i]/FL(1.17);
              outr[i] = outspecr[2*i]/FL(1.17);
            }
        }

        if (phasetrunc) {
          /* setup for fades*/
          if (crossfade || cross) {
            crossout=1;

            for (i = 0; i < complexfftbuff; i+=2) {
              outspecoldl[i] = complexinsig[i]*oldhrtflpad[i] -
                complexinsig[i+1]*oldhrtflpad[i+1];
              outspecoldr[i] = complexinsig[i]*oldhrtfrpad[i] -
                complexinsig[i+1]*oldhrtfrpad[i+1];
              outspecoldl[i+1] = complexinsig[i]*oldhrtflpad[i+1] +
                complexinsig[i+1]*oldhrtflpad[i];
              outspecoldr[i+1] = complexinsig[i]*oldhrtfrpad[i+1] +
                complexinsig[i+1]*oldhrtfrpad[i];
            }

            csound->InverseComplexFFT(csound, outspecoldl, complexIMPLENGTH);
            csound->InverseComplexFFT(csound, outspecoldr, complexIMPLENGTH);

            /*real values, scaled*/
            for (i = 0; i < complexIMPLENGTH; i++)
              {
                outlold[i] = outspecoldl[2*i]/(sr/FL(38000.0));
                outrold[i] = outspecoldr[2*i]/(sr/FL(38000.0));
              }

            cross++;
            /*number of processing buffers in a fade*/
            cross=cross%(int)(fadebuffer/IMPLENGTH);
          }

          if (crossout) {            /*do fade       */
            for (i=0;i<IMPLENGTH;i++) {
              outl[i] = ((outlold[i] + (i<overlapsize ? overlapoldl[i] : 0)) *
                         (FL(1.0) - FL(l)/fadebuffer)) +
                ((outl[i] + (i < overlapsize ? overlapl[i] : 0)) *
                 FL(l)/fadebuffer);
              outr[i] = ((outrold[i] + (i<overlapsize ? overlapoldr[i] : 0)) *
                         (FL(1.0) - FL(l)/fadebuffer)) +
                ((outr[i] + (i < overlapsize ? overlapr[i] : 0)) *
                 FL(l)/fadebuffer);
              l++;
            }
          }
          else
            for (i=0;i<IMPLENGTH;i++)
              {
                outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
                outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
              }

          /*use to check for crossfade next time!*/
          p->oldelevindex = elevindex;
          p->oldangleindex = angleindex;
        }

        if (minphase) {        /*use output direcly and add delay in time domain*/
          for (i=0;i<IMPLENGTH;i++)
            {
              outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : 0);
              outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : 0);
            }

          /*read delay data: 4 nearest points*/
          /*point 1*/
          skip = 0;
          if (elevindexlow == 0);
          else {
            for (i=0; i<elevindexlow; i++)
              skip +=((int)(elevationarray[i]/2)+1);
          }

          if (angleindex1 == 0);
          else {
            for (i=0; i<angleindex1; i++)
              skip++;
          }

          delaylow1=minphasedels[skip];

          /*point 2*/
          skip = 0;
          if (elevindexlow == 0);
          else {
            for (i=0; i<elevindexlow; i++)
              skip +=((int)(elevationarray[i]/2)+1);
          }

          if (angleindex2 == 0);
          else {
            for (i=0; i<angleindex2; i++)
              skip++;
          }

          delaylow2=minphasedels[skip];

          /*point 3*/
          skip = 0;
          if (elevindexhigh == 0);
          else {
            for (i=0; i<elevindexhigh; i++)
              skip +=((int)(elevationarray[i]/2)+1);
          }

          if (angleindex3 == 0);
          else {
            for (i=0; i<angleindex3; i++)
              skip++;
          }

          delayhigh1=minphasedels[skip];

          /*point 4*/
          skip = 0;
          if (elevindexhigh == 0);
          else {
            for (i=0; i<elevindexhigh; i++)
              skip +=((int)(elevationarray[i]/2)+1);
          }

          if (angleindex4 == 0);
          else {
            for (i=0; i<angleindex4; i++)
              skip++;
          }

          delayhigh2=minphasedels[skip];

          /*delay interp*/
          delaylow = delaylow1 +
            ((delaylow2 - delaylow1)*angleindex2per);
          delayhigh = delayhigh1 +
            ((delayhigh2 - delayhigh1)*angleindex4per);
          delayfloat = delaylow +
            ((delayhigh - delaylow)*elevindexhighper);

          /*standard variable delay function for each ear, according
            to interpolated delay value*/
          if (switchchannels) {
            vdtr = (delayfloat + 0.0001f)*sr;
            mdtr = (int)(0.00095f*sr);
            if (vdtr > mdtr) vdtr = (float)mdtr;
            for (i=0;i<IMPLENGTH;i++) {
              rpr = ptr - vdtr;
              if (rpr < 0) rpr += mdtr;
              fracr = rpr - (int) rpr;
              if (rpr  < mdtr -1)
                outvdr =  delmemr[(int)rpr] +
                  fracr*(delmemr[(int)rpr+1] - delmemr[(int)rpr]);
              else outvdr = delmemr[mdtr-1] +
                     fracr*(delmemr[0] - delmemr[mdtr-1]);
              delmemr[ptr] = outr[i];
              outr[i] = outvdr;
              ptr = ptr + 1;
              if (ptr >=mdtr) ptr = 0;
            }

            vdtl = (float)(FL(0.0001)*sr);
            mdtl = (int)(FL(0.00095)*sr);
            if (vdtl > mdtl) vdtl = (float)mdtl;
            for (i=0;i<IMPLENGTH;i++) {
              rpl = ptl - vdtl;
              if (rpl < 0) rpl += mdtl;
              fracl = rpl - (int) rpl;
              if (rpl  < mdtl -1)
                outvdl =  delmeml[(int)rpl] +
                  fracl*(delmeml[(int)rpl+1] - delmeml[(int)rpl]);
              else outvdl = delmeml[mdtl-1] +
                     fracl*(delmeml[0] - delmeml[mdtl-1]);
              delmeml[ptl] = outl[i];
              outl[i] = outvdl;
              ptl = ptl + 1;
              if (ptl >=mdtl) ptl = 0;
            }
          }
          else {
            vdtl = (delayfloat + 0.0001f)*sr;
            mdtl = (int)(.00095f*sr);
            if (vdtl > mdtl) vdtl = (float)mdtl;
            for (i=0;i<IMPLENGTH;i++) {
              rpl = ptl - vdtl;
              if (rpl < 0) rpl += mdtl;
              fracl = rpl - (int) rpl;
              if (rpl  < mdtl -1)
                outvdl =  delmeml[(int)rpl] +
                  fracl*(delmeml[(int)rpl+1] - delmeml[(int)rpl]);
              else outvdl = delmeml[mdtl-1] +
                     fracl*(delmeml[0] - delmeml[mdtl-1]);
              delmeml[ptl] = outl[i];
              outl[i] = outvdl;
              ptl = ptl + 1;
              if (ptl >=mdtl) ptl = 0;
            }

            vdtr = (float)(FL(0.0001)*sr);
            mdtr = (int)(FL(0.00095)*sr);
            if (vdtr > mdtr) vdtr = (float)mdtr;
            for (i=0;i<IMPLENGTH;i++) {
              rpr = ptr - vdtr;
              if (rpr < 0) rpr += mdtr;
              fracr = rpr - (int) rpr;
              if (rpr  < mdtr -1)
                outvdr =  delmemr[(int)rpr] +
                  fracr*(delmemr[(int)rpr+1] - delmemr[(int)rpr]);
              else outvdr = delmemr[mdtr-1] +
                     fracr*(delmemr[0] - delmemr[mdtr-1]);
              delmemr[ptr] = outr[i];
              outr[i] = outvdr;
              ptr = ptr + 1;
              if (ptr >=mdtr) ptr = 0;
            }
          }

          p->ptl = ptl;
          p->ptr = ptr;

        }
        /*reset counter*/
        counter=0;
        if (phasetrunc) {
          /*update*/
          p->cross = cross;
          p->l = l;
        }

      }       /* end of IMPLENGTH == counter*/

    }   /* end of ksmps audio loop */

        /*update*/
    p->counter = counter;
    if (phasetrunc)p->initialfade = initialfade;

    return OK;
}



/*Csound hrtf magnitude interpolation, woodworth phase, static source: January 08*/
/*overlap add convolution*/

/*aleft, aright hrtfstat ain, iang, iel, ifilel, ifiler [,iradius = 9.0,
                         isr = 44100]...options of 48 and 96k sr*/

/*definitions above*/
typedef struct {
  OPDS  h;
  MYFLT *outsigl, *outsigr;
  MYFLT *in, *iangle, *ielev, *ifilel,
    *ifiler, *oradius, *osr;   /* outputs and inputs*/

  int IMPLENGTH, complexIMPLENGTH,
    overlapsize, complexfftbuff;        /*see definitions in INIT*/
  MYFLT sroverN;

  int counter;
  MYFLT sr;

  AUXCH hrtflpad,hrtfrpad;              /*hrtf data padded*/
  AUXCH insig, outl, outr;              /*in and output buffers*/

  /*memory local to perform method*/
  AUXCH complexinsig;                   /*insig fft*/
  AUXCH hrtflfloat, hrtfrfloat;         /*hrtf buffers (rectangular complex form)*/
  AUXCH outspecl, outspecr;             /*spectral data*/

  AUXCH overlapl, overlapr;             /*overlap data*/

  AUXCH lowl1,lowr1,lowl2,lowr2,
    highl1,highr1,highl2,highr2;        /*interpolation buffers*/

  AUXCH leftshiftbuffer,rightshiftbuffer;  /*buffers for impulse shift*/

} hrtfstat;

static int hrtfstat_init(CSOUND *csound, hrtfstat *p)
{
    /*left and right data files: spectral mag, phase format.*/
    MEMFIL *fpl=NULL,*fpr=NULL;
    char filel[MAXNAME],filer[MAXNAME];

    /*interpolation values*/
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

    MYFLT elev = *p->ielev;
    MYFLT angle = *p->iangle;
    MYFLT r = *p->oradius;
    MYFLT sr = *p->osr;

    float *fpindexl=NULL;         /* pointers into HRTF files*/
    float *fpindexr=NULL;

    int IMPLENGTH = 0;                /*time domain impulse length */
    int complexIMPLENGTH = 0;         /*freq domain impulse length*/
    int overlapsize = 0;              /*overlap add convolution*/
    int complexfftbuff = 0;           /*complex fft used(min phase needs it)*/

    int i,elevindex, angleindex, switchchannels=0, skip=0;

    /*local interpolation values*/
    MYFLT elevindexlowper,elevindexhighper,angleindex1per,
      angleindex2per,angleindex3per,angleindex4per;
    int elevindexlow,elevindexhigh,angleindex1,angleindex2,angleindex3,angleindex4;
    MYFLT magl,magr,phasel,phaser, magllow, magrlow, maglhigh, magrhigh;

    /*woodworth values*/
    float radianangle,radianelev,itd,freq;

    /*shift stuff*/
    int shift;
    MYFLT *leftshiftbuffer;
    MYFLT *rightshiftbuffer;

    /*woodworth: change for different sr*/

    if (sr!=FL(44100.0)&&sr!=FL(48000.0)&&sr!=FL(96000.0))
      sr=FL(44100.0);
    p->sr = sr;

    if (UNLIKELY(csound->esr != sr))
      csound->Warning(csound,
                      Str("Orchestra sampling rate is not "
                          "compatible with HRTF data files\nShould be %.0f,"
                          " see Csound help for object\n\n"), sr);

    strcpy(filel, (char*) p->ifilel);             /*copy in string name...*/
    strcpy(filer, (char*) p->ifiler);

    if (sr == FL(44100.0)) {
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    else if (sr == FL(48000.0)) {
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    else if (sr == FL(96000.0)) {
      IMPLENGTH = 256;
      complexIMPLENGTH = 512;
      overlapsize = (IMPLENGTH-1);
      complexfftbuff = (complexIMPLENGTH*2);

      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    p->IMPLENGTH = IMPLENGTH;
    p->complexIMPLENGTH = complexIMPLENGTH;
    p->overlapsize = overlapsize;
    p->complexfftbuff = complexfftbuff;

    p->sroverN = sr/IMPLENGTH;

    if (fpl && fpr) {
      /*start indices at correct value (start of file)/ zero indices.*/
      fpindexl = (float *) fpl->beginp;
      fpindexr = (float *) fpr->beginp;
    }
    /*buffers */
    csound->AuxAlloc(csound, IMPLENGTH*sizeof(MYFLT), &p->insig);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outr);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->hrtflpad);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->hrtfrpad);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p-> complexinsig);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtflfloat);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtfrfloat);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecl);
    csound->AuxAlloc(csound, complexfftbuff*sizeof(MYFLT), &p->outspecr);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapl);
    csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapr);

    /*interpolation values*/
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr2);

    /*shift buffers*/
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->leftshiftbuffer);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->rightshiftbuffer);

    lowl1 = (MYFLT *)p->lowl1.auxp;
    lowr1 = (MYFLT *)p->lowr1.auxp;
    lowl2 = (MYFLT *)p->lowl2.auxp;
    lowr2 = (MYFLT *)p->lowr2.auxp;
    highl1 = (MYFLT *)p->highl1.auxp;
    highr1 = (MYFLT *)p->highr1.auxp;
    highl2 = (MYFLT *)p->highl2.auxp;
    highr2 = (MYFLT *)p->highr2.auxp;

    leftshiftbuffer = (MYFLT *)p->leftshiftbuffer.auxp;
    rightshiftbuffer = (MYFLT *)p->rightshiftbuffer.auxp;

    hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;

    hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    if (r<=0||r>15)
      r=9.0;

    if (elev < -40) elev = -40;            /*within legal MIT range*/
    if (elev > 90) elev = 90;
    elevindex = (int)floorf(((float)(elev-minelev)/elevincrement)+0.5f);

    while (angle<0)angle+=360;
    while (angle>360)angle-=360;                                   /*mod 360*/
    if (angle>180) {
      angle=360-angle;
      switchchannels=1;         /*true for later function: data is symmetrical*/
    }

    /*read using an index system based on number of points measured
      per elevation at mit*/
    angleindex = (int)floorf(angle/(FL(360.0)/(MYFLT)elevationarray[elevindex])+FL(0.5));
    /*angle/increment+0.5*/
    if (angleindex>=((int)(elevationarray[elevindex]/2)+1))
      /*last point in current elevation*/
      angleindex=(int)(elevationarray[elevindex]/2);

    /* two nearest elev indices*/
    elevindexlow = (int)((elev-minelev)/elevincrement);
    if (elevindexlow<13) elevindexhigh = elevindexlow+1;
    else elevindexhigh = elevindexlow;            /* highest index reached*/

    /* get percentage value for interpolation*/
    elevindexlowper = FL(1.0) -
      (MYFLT)(((MYFLT)(elev-minelev)/elevincrement)-(MYFLT)elevindexlow);
    elevindexhighper = FL(1.0) - elevindexlowper;

    /* 4 closest angle indices, 2 low and 2 high*/
    angleindex1 = (int)(angle/(360.0f/(float)elevationarray[elevindexlow]));

    angleindex2 = angleindex1 + 1;
    if (angleindex2>=((int)(elevationarray[elevindexlow]/2)+1))
      angleindex2=(int)(elevationarray[elevindexlow]/2);

    angleindex3 = (int)(angle/(360.0f/(float)elevationarray[elevindexhigh]));

    angleindex4 = angleindex3 + 1;
    if (angleindex4>=((int)(elevationarray[elevindexhigh]/2)+1))
      angleindex4=(int)(elevationarray[elevindexhigh]/2);

    /* angle percentages for interp*/
    angleindex1per = FL(1.0) -
      (MYFLT)((angle/(FL(360.0)/(MYFLT)elevationarray[elevindexlow])-angleindex1));
    angleindex2per = FL(1.0)-angleindex1per;
    angleindex3per = FL(1.0) -
      (MYFLT)((angle/(360.0/(MYFLT)elevationarray[elevindexhigh])-angleindex3));
    angleindex4per = FL(1.0)-angleindex3per;

    /*read 4 nearest points for interpolation*/
    /*point 1*/
    skip = 0;
    if (elevindexlow == 0);
    else
      for (i=0; i<elevindexlow; i++)
        /*skip * 2 as data is in complex mag,phase format*/
        skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

    if (angleindex1 == 0);
    else
      for (i=0; i<angleindex1; i++)
        skip += (2*IMPLENGTH);

    if (switchchannels) {
      for (i=0;i<complexIMPLENGTH;i++) {
        lowl1[i]=fpindexr[skip+i];
        lowr1[i]=fpindexl[skip+i];
      }
    }

    else {
      for (i=0;i<complexIMPLENGTH;i++) {
        lowl1[i]=fpindexl[skip+i];
        lowr1[i]=fpindexr[skip+i];
      }
    }

    /*point 2*/
    skip = 0;
    if (elevindexlow == 0);
    else
      for (i=0; i<elevindexlow; i++)
        skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

    if (angleindex2 == 0);
    else
      for (i=0; i<angleindex2; i++)
        skip += (2*IMPLENGTH);

    if (switchchannels) {
      for (i=0;i<complexIMPLENGTH;i++) {
        lowl2[i]=fpindexr[skip+i];
        lowr2[i]=fpindexl[skip+i];
      }
    }

    else {
      for (i=0;i<complexIMPLENGTH;i++) {
        lowl2[i]=fpindexl[skip+i];
        lowr2[i]=fpindexr[skip+i];
      }
    }

    /*point 3*/
    skip = 0;
    if (elevindexhigh == 0);
    else
      for (i=0; i<elevindexhigh; i++)
        skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

    if (angleindex3 == 0);
    else
      for (i=0; i<angleindex3; i++)
        skip += (2*IMPLENGTH);

    if (switchchannels) {
      for (i=0;i<complexIMPLENGTH;i++) {
        highl1[i]=fpindexr[skip+i];
        highr1[i]=fpindexl[skip+i];
      }
    }

    else {
      for (i=0;i<complexIMPLENGTH;i++) {
        highl1[i]=fpindexl[skip+i];
        highr1[i]=fpindexr[skip+i];
      }
    }

    /*point 4*/
    skip = 0;
    if (elevindexhigh == 0);
    else
      for (i=0; i<elevindexhigh; i++)
        skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

    if (angleindex4 == 0);
    else
      for (i=0; i<angleindex4; i++)
        skip += (2*IMPLENGTH);

    if (switchchannels) {
      for (i=0;i<complexIMPLENGTH;i++) {
        highl2[i]=fpindexr[skip+i];
        highr2[i]=fpindexl[skip+i];
      }
    }

    else {
      for (i=0;i<complexIMPLENGTH;i++) {
        highl2[i]=fpindexl[skip+i];
        highr2[i]=fpindexr[skip+i];
      }
    }

    /*woodworth stuff*/
    radianangle = angle * (float)PI/180.0f;  /* degrees to radians */
    radianelev = elev * (float)PI/180.0f;

    if (radianangle > (PI/2.0))             /*get in correct range for formula*/
      radianangle = (float)PI - radianangle;

    /*woodworth formula for itd*/
    itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;

    /* magnitude interpolation*/
    for (i=0; i < complexIMPLENGTH; i+=2) {
      /* interpolate high and low mags*/
      magllow = lowl1[i]+(MYFLT)((lowl2[i]-lowl1[i])*(MYFLT)angleindex2per);
      maglhigh = highl1[i]+(MYFLT)((highl2[i]-highl1[i])*(MYFLT)angleindex4per);

      magrlow = lowr1[i]+(MYFLT)((lowr2[i]-lowr1[i])*(MYFLT)angleindex2per);
      magrhigh = highr1[i]+(MYFLT)((highr2[i]-highr1[i])*(MYFLT)angleindex4per);

      /* interpolate high and low results*/
      magl = magllow+(MYFLT)((maglhigh-magllow)*(MYFLT)elevindexhighper);
      magr = magrlow+(MYFLT)((magrhigh-magrlow)*(MYFLT)elevindexhighper);

      if (i<(IMPLENGTH+1))freq = (i/2)*p->sroverN;
      else freq = (-IMPLENGTH+(i/2))*p->sroverN;

      /*NONLIN ITD...*/
      if (p->sr == 96000) {
        if ((i/2)>0 && (i/2)<6) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd96k[(i/2)-1];
        }
        if ((i/2)>251 && (i/2)<256) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd96k[255-(i/2)];
        }
      }
      if (p->sr == 48000) {
        if ((i/2)>0 && (i/2)<6) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd48k[(i/2)-1];
        }
        if ((i/2)>123 && (i/2)<128) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd48k[127-(i/2)];
        }
      }
      if (p->sr == 44100) {
        if ((i/2)>0 && (i/2)<6) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd[(i/2)-1];
        }
        if ((i/2)>123 && (i/2)<128) {
          itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;
          itd = itd * nonlinitd[127-(i/2)];
        }
      }

      if (switchchannels) {
        phasel = TWOPI_F*freq*(itd/2);
        phaser = TWOPI_F*freq*-(itd/2);
      }
      else {
        phasel = TWOPI_F*freq*-(itd/2);
        phaser = TWOPI_F*freq*(itd/2);
      }

      /* polar to rectangular*/
      hrtflfloat[i] = magl*COS(phasel);
      hrtflfloat[i+1] = magl*SIN(phasel);

      hrtfrfloat[i] = magr*COS(phaser);
      hrtfrfloat[i+1] = magr*SIN(phaser);
    }

    /*ifft*/
    csound->InverseComplexFFT(csound, hrtflfloat, IMPLENGTH);
    csound->InverseComplexFFT(csound, hrtfrfloat, IMPLENGTH);

    for (i=0; i<complexIMPLENGTH; i++) {
      hrtflpad[i] = hrtflfloat[i]; /* scale and pad buffers with zeros to fftbuff*/
      hrtfrpad[i] = hrtfrfloat[i];
    }

    /*shift for causality...impulse as is is centred around zero time lag...
      then phase added.*/
    /*this step centres impulse around centre tap of filter (then phase
      moves it for correct itd...)*/
    shift = IMPLENGTH;
    for (i=0;i<complexIMPLENGTH;i++) {
      leftshiftbuffer[i] = hrtflpad[i];
      rightshiftbuffer[i] = hrtfrpad[i];
    }

    for (i=0;i<complexIMPLENGTH;i++) {
      hrtflpad[i] = leftshiftbuffer[shift];
      hrtfrpad[i] = rightshiftbuffer[shift];

      shift++;
      shift = shift%complexIMPLENGTH;
    }

    /*zero pad impulse*/
    for (i=complexIMPLENGTH;i<complexfftbuff;i++) {
      hrtflpad[i]=FL(0.0);
      hrtfrpad[i]=FL(0.0);
    }

    /*back to freq domain*/
    csound->ComplexFFT(csound, hrtflpad, complexIMPLENGTH);
    csound->ComplexFFT(csound, hrtfrpad, complexIMPLENGTH);


    p->counter = 0;               /*initialize counter*/

    return OK;
}


static int hrtfstat_process(CSOUND *csound, hrtfstat *p)
{
    MYFLT *in = p->in;                    /*local pointers to p*/
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /*common buffers and variables*/
    MYFLT *insig = (MYFLT *)p->insig.auxp;
    MYFLT *outl = (MYFLT *)p->outl.auxp;
    MYFLT *outr = (MYFLT *)p->outr.auxp;

    MYFLT *hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    MYFLT *hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT *overlapl = (MYFLT *)p->overlapl.auxp;
    MYFLT *overlapr = (MYFLT *)p->overlapr.auxp;

    int counter = p->counter;
    int n,j,i;

    int IMPLENGTH = p->IMPLENGTH;
    int complexIMPLENGTH = p->complexIMPLENGTH;
    int overlapsize = p->overlapsize;
    int complexfftbuff = p->complexfftbuff;

    MYFLT sr = p->sr;

    n=csound->ksmps;

    for (j=0;j<n;j++) {
      insig[counter] = in[j];                   /*ins and outs*/

      outsigl[j]=outl[counter];
      outsigr[j]=outr[counter];

      counter++;

      if (counter == IMPLENGTH) {        /*process a block*/
        /* look after overlap add stuff*/
        for (i = 0; i < overlapsize ; i++) {
          overlapl[i] = outl[i+IMPLENGTH];
          overlapr[i] = outr[i+IMPLENGTH];
        }

        /* insert insig for complex real,im fft, zero pad*/
        for (i = 0; i <  IMPLENGTH; i++) {
          complexinsig[2*i] = insig[i];
          complexinsig[(2*i)+1] = FL(0.0);
        }

        for (i = complexIMPLENGTH; i <  complexfftbuff; i++)
          complexinsig[i] = FL(0.0);

        csound->ComplexFFT(csound, complexinsig, complexIMPLENGTH);

        /* complex multiplication*/
        for (i = 0; i < complexfftbuff; i+=2) {
          outspecl[i] = complexinsig[i]*hrtflpad[i] -
            complexinsig[i+1]*hrtflpad[i+1];
          outspecr[i] = complexinsig[i]*hrtfrpad[i] -
            complexinsig[i+1]*hrtfrpad[i+1];
          outspecl[i+1] = complexinsig[i]*hrtflpad[i+1] +
            complexinsig[i+1]*hrtflpad[i];
          outspecr[i+1] = complexinsig[i]*hrtfrpad[i+1] +
            complexinsig[i+1]*hrtfrpad[i];
        }

        /* convolution is the inverse FFT of above result */
        csound->InverseComplexFFT(csound, outspecl, complexIMPLENGTH);
        csound->InverseComplexFFT(csound, outspecr, complexIMPLENGTH);


        /*real values*/

        /*scaled by a factor related to sr...?*/

        for (i = 0; i < complexIMPLENGTH; i++) {
          outl[i] = outspecl[2*i]/(sr/FL(38000.0));
          outr[i] = outspecr[2*i]/(sr/FL(38000.0));
        }


        for (i=0;i<IMPLENGTH;i++) {
          outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
          outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
        }

        /*reset counter*/
        counter=0;

      }       /* end of IMPLENGTH == counter*/

    }   /* end of ksmps audio loop */

        /*update*/
    p->counter = counter;

    return OK;
}

/*Csound hrtf magnitude interpolation, dynamic woodworth trajectory: September 07*/
/*stft from fft.cpp in sndobj...*/

/*****/
/*stft based on sndobj implementation...some notes:*/
/*need an overlapskip (same as m_counter) for in and out to control seperately...*/
/*ifft.cpp zeros current overlapskipout,then decrements t...not possible
  here as t needs to be decremented before fft...*/
/*****/

/*aleft, aright hrtfmove2 ain, kang, kel, ifilel, ifiler
                          [,ioverlap = 4, iradius = 9.0, isr = 44100] */
/*ioverlap is stft overlap, iradius is head radius, sr can also be 48000 and 96000*/

/*definitions above*/
typedef struct {
  OPDS  h;
  MYFLT *outsigl, *outsigr;
  MYFLT *in, *kangle, *kelev, *ifilel, *ifiler,
    *ooverlap, *oradius, *osr;   /* outputs and inputs */

  int IMPLENGTH, complexIMPLENGTH, complexfftbuff; /*see definitions in INIT*/
  MYFLT sroverN;
  MYFLT sr;

  /*test inputs in init, get accepted value/default, and store in variables below.*/
  int overlap;
  MYFLT radius;

  int hopsize;

  MEMFIL *fpl,*fpr;                     /* file pointers*/
  float *fpbeginl,*fpbeginr;

  int counter, t;                       /*to keep track of process*/

  AUXCH inbuf;          /*in and output buffers*/
  AUXCH outbufl, outbufr;

  /*memory local to perform method*/
  AUXCH complexinsig;                   /*insig fft*/
  AUXCH hrtflfloat, hrtfrfloat;         /*hrtf buffers (rectangular complex form)*/
  AUXCH outspecl, outspecr;             /*spectral data*/

  AUXCH lowl1,lowr1,lowl2,lowr2,
        highl1,highr1,highl2,highr2;    /*interpolation buffers*/

  AUXCH leftshiftbuffer,rightshiftbuffer; /*buffers for impulse shift*/

  AUXCH win;            /*stft window*/
  AUXCH overlapskipin,overlapskipout;  /*used for skipping into next stft
                                         array on way in and out*/

} hrtfmove2;

static int hrtfmove2_init(CSOUND *csound, hrtfmove2 *p)
{
    /*left and right data files: spectral mag, phase format.*/
    MEMFIL *fpl=NULL,*fpr=NULL;

    char filel[MAXNAME],filer[MAXNAME];

    int IMPLENGTH;              /*time domain impulse length */
    int complexIMPLENGTH;       /*freq domain impulse length*/

    MYFLT *win;                        /*stft window*/
    int *overlapskipin, *overlapskipout; /*overlap skip buffers*/
    MYFLT *inbuf;
    MYFLT *outbufl, *outbufr;

    int overlap = (int)*p->ooverlap;
    MYFLT r = *p->oradius;
    MYFLT sr = *p->osr;

    int i=0;

    if (sr!=FL(44100.0)&&sr!=FL(48000.0)&&sr!=FL(96000.0))
      sr=FL(44100.0);
    p->sr = sr;

    if (UNLIKELY(csound->esr != sr))
      csound->Warning(csound,
                      Str("Orchestra sampling rate is not "
                          "compatible with HRTF data files\nShould be %.0f, "
                          "see Csound help for object\n\n"), sr);

    strcpy(filel, (char*) p->ifilel);             /*copy in string name...*/
    strcpy(filer, (char*) p->ifiler);

    if (sr == FL(44100.0)) {             /*set up per sr...*/
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;

      /*  csound->Message(csound,"\ndatafile:44.1k\n");*/
      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    else if (sr == FL(48000.0)) {
      IMPLENGTH = 128;
      complexIMPLENGTH = 256;

      /*csound->Message(csound,"\ndatafile:48k\n");*/
      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }

    else if (sr == FL(96000.0)) {
      IMPLENGTH = 256;
      complexIMPLENGTH = 512;

      /*csound->Message(csound,"\ndatafile:96k\n");*/
      if (UNLIKELY((fpl = csound->ldmemfile2withCB(csound, filel,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load left data file, exiting\n\n"));
      }

      if (UNLIKELY((fpr = csound->ldmemfile2withCB(csound, filer,
                                                   CSFTYPE_FLOATS_BINARY,
                                                   swap4bytes)) == NULL)) {
        return
          csound->InitError(csound,
                            Str("\n\n\ncannot load right data file, exiting\n\n"));
      }
    }
    else {
      return
        csound->InitError(csound,
                          Str("\n\n\n Sampling rate not supported, exiting\n\n"));
    }

    p->IMPLENGTH = IMPLENGTH;
    p->complexIMPLENGTH = complexIMPLENGTH;

    p->sroverN = sr/IMPLENGTH;

    /*file handles*/
    if (fpl && fpr) {
      p->fpl = fpl;
      p->fpr = fpr;
      p->fpbeginl = (float *) fpl->beginp;
      p->fpbeginr = (float *) fpr->beginp;
    }

    if (overlap!=2&&overlap!=4&&overlap!=8&&overlap!=16)
      overlap=4;
    p->overlap = overlap;

    if (r<=0||r>15)
      r=9.0;
    p->radius = r;

    p->hopsize = (int)(IMPLENGTH/overlap);

    /*buffers */
    csound->AuxAlloc(csound, (overlap*IMPLENGTH)*sizeof(MYFLT), &p->inbuf);
  /*2d arrays in 1d!*/
    csound->AuxAlloc(csound, (overlap*IMPLENGTH)*sizeof(MYFLT), &p->outbufl);
    csound->AuxAlloc(csound, (overlap*IMPLENGTH)*sizeof(MYFLT), &p->outbufr);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p-> complexinsig);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtflfloat);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->hrtfrfloat);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outspecl);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->outspecr);

    /*interpolation values*/
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->lowr2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr1);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highl2);
    csound->AuxAlloc(csound, complexIMPLENGTH*sizeof(MYFLT), &p->highr2);

    csound->AuxAlloc(csound, IMPLENGTH*sizeof(MYFLT), &p->win);
    csound->AuxAlloc(csound, overlap*sizeof(int), &p->overlapskipin);
    csound->AuxAlloc(csound, overlap*sizeof(int), &p->overlapskipout);

    win = (MYFLT *)p->win.auxp;
    overlapskipin = (int *)p->overlapskipin.auxp;
    overlapskipout = (int *)p->overlapskipout.auxp;
    inbuf = (MYFLT *)p->inbuf.auxp;
    outbufl = (MYFLT *)p->outbufl.auxp;
    outbufr = (MYFLT *)p->outbufr.auxp;

    for (i=0; i< (overlap*IMPLENGTH); i++) {
      inbuf[i] = FL(0.0);
      outbufl[i] = FL(0.0);
      outbufr[i] = FL(0.0);
    }

    /* window is hanning*/
    for (i=0; i< IMPLENGTH; i++)
      win[i] = FL(0.5) - (MYFLT)(0.5*cos(i*TWOPI/(double)(IMPLENGTH-1)));

    for (i = 0; i < overlap; i++) {
      /*so, for example in overlap 4: will be 0,32,64,96*/
      overlapskipin[i] = p->hopsize*i;
      overlapskipout[i] = p->hopsize*i;
    }

    p->counter = 0;               /*initialise*/
    p->t = 0;

    return OK;
}


static int hrtfmove2_process(CSOUND *csound, hrtfmove2 *p)
{
    MYFLT *in = p->in;                    /*local pointers to p*/
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /*common buffers and variables*/
    MYFLT *inbuf = (MYFLT *)p->inbuf.auxp;

    MYFLT *outbufl = (MYFLT *)p->outbufl.auxp;
    MYFLT *outbufr = (MYFLT *)p->outbufr.auxp;

    MYFLT outsuml=FL(0.0), outsumr=FL(0.0);

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    MYFLT *hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT elev = *p->kelev;
    MYFLT angle = *p->kangle;
    int overlap = p->overlap;
    MYFLT r = p->radius;

    MYFLT sr = p->sr;
    MYFLT sroverN = p->sroverN;

    int hopsize = p->hopsize;

    MYFLT *win = (MYFLT *)p->win.auxp;
    int *overlapskipin = (int *)p->overlapskipin.auxp;
    int *overlapskipout = (int *)p->overlapskipout.auxp;

    int counter = p ->counter;
    int t = p ->t;
    int n;

    float *fpindexl;                      /* pointers into HRTF files*/
    float *fpindexr;

    int i,j,elevindex, angleindex, switchchannels=0, skip=0;

    /*interpolation values*/
    MYFLT *lowl1 = (MYFLT *)p->lowl1.auxp;
    MYFLT *lowr1 = (MYFLT *)p->lowr1.auxp;
    MYFLT *lowl2 = (MYFLT *)p->lowl2.auxp;
    MYFLT *lowr2 = (MYFLT *)p->lowr2.auxp;
    MYFLT *highl1 = (MYFLT *)p->highl1.auxp;
    MYFLT *highr1 = (MYFLT *)p->highr1.auxp;
    MYFLT *highl2 = (MYFLT *)p->highl2.auxp;
    MYFLT *highr2 = (MYFLT *)p->highr2.auxp;

    /*local interpolation values*/
    MYFLT elevindexlowper,elevindexhighper,angleindex1per,
      angleindex2per,angleindex3per,angleindex4per;
    int elevindexlow,elevindexhigh,angleindex1,angleindex2,
      angleindex3,angleindex4;
    MYFLT magl,magr,phasel,phaser, magllow, magrlow, maglhigh, magrhigh;

    /*woodworth values*/
    float radianangle,radianelev,itd,freq;

    int IMPLENGTH = p->IMPLENGTH;
    int complexIMPLENGTH = p->complexIMPLENGTH;
    /*int complexfftbuff = p->complexfftbuff;*/

    /*start indices at correct value (start of file)/ zero indices.*/
    fpindexl = (float *) p->fpbeginl;
    fpindexr = (float *) p->fpbeginr;

    n=csound->ksmps;

    /*ksmps loop*/
    for (j=0;j<n;j++) {

      /* distribute the signal and apply the window*/
      /* according to a time pointer (kept by overlapskip[n])*/
      for (i=0;i < overlap; i++) {
        inbuf[(i*IMPLENGTH)+overlapskipin[i]]= in[j]*win[overlapskipin[i]];
        overlapskipin[i]++;
      }

      counter++;

      if (counter==hopsize) {            /*process a block*/

        if (elev < -40) elev = -40;            /*within legal MIT range*/
        if (elev > 90) elev = 90;
        elevindex = (int)floor((float)((float)(elev-minelev)/elevincrement)+0.5);

        while (angle<0)angle+=360;
        while (angle>360)angle-=360;                                   /*mod 360*/
        if (angle>180) {
          angle=360-angle;
          switchchannels=1; /*true for later function: data is symmetrical*/
        }

        /*read using an index system based on number of points measured
          per elevation at MIT*/
        angleindex = (int)floor(angle/(360/(MYFLT)elevationarray[elevindex])+0.5);
        /*angle/increment+0.5*/
        if (angleindex>=((int)(elevationarray[elevindex]/2)+1))
          /*last point in current elevation*/
          angleindex=(int)(elevationarray[elevindex]/2);

        /* two nearest elev indices*/
        elevindexlow = (int)((elev-minelev)/elevincrement);
        if (elevindexlow<13) elevindexhigh = elevindexlow+1;
        else elevindexhigh = elevindexlow;            /* highest index reached*/

        /* get percentage value for interpolation*/
        elevindexlowper = (FL(1.0) -
                           (MYFLT)(((MYFLT)(elev-minelev)/elevincrement)-
                                   (MYFLT)elevindexlow));
        elevindexhighper = FL(1.0) - elevindexlowper;

        /* 4 closest angle indices, 2 low and 2 high*/
        angleindex1 = (int)(angle/(360/(float)elevationarray[elevindexlow]));

        angleindex2 = angleindex1 + 1;
        if (angleindex2>=((int)(elevationarray[elevindexlow]/2)+1))
          angleindex2=(int)(elevationarray[elevindexlow]/2);

        angleindex3 = (int)(angle/(360/(float)elevationarray[elevindexhigh]));

        angleindex4 = angleindex3 + 1;
        if (angleindex4>=((int)(elevationarray[elevindexhigh]/2)+1))
          angleindex4=(int)(elevationarray[elevindexhigh]/2);

        /* angle percentages for interp*/
        angleindex1per =
          (FL(1.0)-(MYFLT)((angle/(FL(360.0)/(MYFLT)elevationarray[elevindexlow])
                            -angleindex1)));
        angleindex2per = FL(1.0)-angleindex1per;
        angleindex3per =
          (FL(1.0) - (MYFLT)((angle/(360.0/(MYFLT)elevationarray[elevindexhigh])-
                              angleindex3)));
        angleindex4per = 1.0f-angleindex3per;

        /*read 4 nearest points for interpolation*/
        /*point 1*/
        skip = 0;
        if (elevindexlow == 0);
        else
          for (i=0; i<elevindexlow; i++)
            /*skip * 2 as data is in complex mag,phase format*/
            skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex1 == 0);
        else
          for (i=0; i<angleindex1; i++)
            skip += (2*IMPLENGTH);
        
        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl1[i]=fpindexr[skip+i];
            lowr1[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl1[i]=fpindexl[skip+i];
            lowr1[i]=fpindexr[skip+i];
          }
        }

        /*point 2*/
        skip = 0;
        if (elevindexlow == 0);
        else
          for (i=0; i<elevindexlow; i++)
            skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex2 == 0);
        else
          for (i=0; i<angleindex2; i++)
            skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl2[i]=fpindexr[skip+i];
            lowr2[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            lowl2[i]=fpindexl[skip+i];
            lowr2[i]=fpindexr[skip+i];
          }
        }

        /*point 3*/
        skip = 0;
        if (elevindexhigh == 0);
        else
          for (i=0; i<elevindexhigh; i++)
            skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex3 == 0);
        else
          for (i=0; i<angleindex3; i++)
            skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl1[i]=fpindexr[skip+i];
            highr1[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl1[i]=fpindexl[skip+i];
            highr1[i]=fpindexr[skip+i];
          }
        }

        /*point 4*/
        skip = 0;
        if (elevindexhigh == 0);
        else
          for (i=0; i<elevindexhigh; i++)
            skip +=((int)(elevationarray[i]/2)+1)*(IMPLENGTH*2);

        if (angleindex4 == 0);
        else
          for (i=0; i<angleindex4; i++)
            skip += (2*IMPLENGTH);

        if (switchchannels) {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl2[i]=fpindexr[skip+i];
            highr2[i]=fpindexl[skip+i];
          }
        }

        else {
          for (i=0;i<complexIMPLENGTH;i++) {
            highl2[i]=fpindexl[skip+i];
            highr2[i]=fpindexr[skip+i];
          }
        }

        /*woodworth stuff*/
        radianangle = angle * (float)PI/180.0f; /*degrees to radians*/
        radianelev = elev * (float)PI/180.0f;

        if (radianangle > (PI/2.0))
          /*get in correct range for formula*/
          radianangle = (float)PI - radianangle;

        /*woodworth formula for itd     */
        itd = (radianangle + sinf(radianangle))*r*cosf(radianelev)/c;

        /* magnitude interpolation*/
        for (i=0; i < complexIMPLENGTH; i+=2) {
          /* interpolate high and low mags*/
          magllow = lowl1[i]+(MYFLT)((lowl2[i]-lowl1[i])*(MYFLT)angleindex2per);
          maglhigh = highl1[i]+(MYFLT)((highl2[i]-highl1[i])*(MYFLT)angleindex4per);

          magrlow = lowr1[i]+(MYFLT)((lowr2[i]-lowr1[i])*(MYFLT)angleindex2per);
          magrhigh = highr1[i]+(MYFLT)((highr2[i]-highr1[i])*(MYFLT)angleindex4per);

          /* interpolate high and low results*/
          magl = magllow+(MYFLT)((maglhigh-magllow)*(MYFLT)elevindexhighper);
          magr = magrlow+(MYFLT)((magrhigh-magrlow)*(MYFLT)elevindexhighper);

          if (i<(IMPLENGTH+1))freq = (i/2)*sroverN;
          else freq = (-IMPLENGTH+(i/2))*sroverN;

          /*NONLIN ITD HERE!...NEED SEPERATE ARRAY FOR 48 and 96k...*/
          if (p->sr == FL(96000.0)) {
            if ((i/2)>0 && (i/2)<6) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd96k[(i/2)-1];
            }
            if ((i/2)>251 && (i/2)<256) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd96k[255-(i/2)];
            }
          }
          if (p->sr == FL(48000.0)) {
            if ((i/2)>0 && (i/2)<6) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd48k[(i/2)-1];
            }
            if ((i/2)>123 && (i/2)<128) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd48k[127-(i/2)];
            }
          }
          if (p->sr == FL(44100.0)) {
            if ((i/2)>0 && (i/2)<6) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd[(i/2)-1];
            }
            if ((i/2)>123 && (i/2)<128) {
              itd = (radianangle +
                     sinf(radianangle))*r*cosf(radianelev)/c;
              itd = itd * nonlinitd[127-(i/2)];
            }
          }

          if (switchchannels) {
            phasel = TWOPI_F*freq*(itd/2);
            phaser = TWOPI_F*freq*-(itd/2);
          }
          else {
            phasel = TWOPI_F*freq*-(itd/2);
            phaser = TWOPI_F*freq*(itd/2);
          }

          /* polar to rectangular*/
          hrtflfloat[i] = magl*COS(phasel);
          hrtflfloat[i+1] = magl*SIN(phasel);

          hrtfrfloat[i] = magr*COS(phaser);
          hrtfrfloat[i+1] = magr*SIN(phaser);
        }

        /* t used to read inbuf...*/
        t--; if (t<0) t = overlap-1;

        /* insert insig for complex real,im fft               */
        for (i=0;i<IMPLENGTH;i++) {
          complexinsig[2*i]=inbuf[(t*IMPLENGTH)+i];
          complexinsig[(2*i)+1]=FL(0.0);
        }

        /* zero the current input sigframe time pointer*/
        overlapskipin[t] = 0;

        csound->ComplexFFT(csound, complexinsig, IMPLENGTH);

        /* complex multiplication*/
        for (i = 0; i < complexIMPLENGTH; i+=2) {
          outspecl[i] = complexinsig[i]*hrtflfloat[i] -
            complexinsig[i+1]*hrtflfloat[i+1];
          outspecr[i] = complexinsig[i]*hrtfrfloat[i] -
            complexinsig[i+1]*hrtfrfloat[i+1];
          outspecl[i+1] = complexinsig[i]*hrtflfloat[i+1] +
            complexinsig[i+1]*hrtflfloat[i];
          outspecr[i+1] = complexinsig[i]*hrtfrfloat[i+1] +
            complexinsig[i+1]*hrtfrfloat[i];
        }

        /* convolution is the inverse FFT of above result */
        csound->InverseComplexFFT(csound, outspecl, IMPLENGTH);
        csound->InverseComplexFFT(csound, outspecr, IMPLENGTH);

        /*real values, scaled*/

        /*Should be simply scaled by rms of window(.707)?...however
          need scaling based on overlap (more overlaps -> louder) and sr...?*/
        for (i = 0; i < IMPLENGTH; i++) {
          outbufl[(t*IMPLENGTH)+i] =
            outspecl[2*i] / (overlap*FL(0.5)*(sr/FL(44100.0)));
          outbufr[(t*IMPLENGTH)+i] =
            outspecr[2*i] / (overlap*FL(0.5)*(sr/FL(44100.0)));
        }

      }       /* end of !counter%hopsize*/

      /*output = sum of all relevant outputs: eg if overlap = 4 and counter = 0, */
      /*outsigl[j] = outbufl[0] + outbufl[128+96] +
                     outbufl[256+64] + outbufl[384+32];*/
      /*        * * * * [ ]                     +*/
      /*          * * * [*]             +*/
      /*            * * [*] *           +*/
      /*              * [*] * *         =*/
      /*stft!*/

      outsuml = outsumr = FL(0.0);

      for (i=0;i<(int)overlap;i++) {
        outsuml += outbufl[(i*IMPLENGTH)+overlapskipout[i]]*win[overlapskipout[i]];
        outsumr += outbufr[(i*IMPLENGTH)+overlapskipout[i]]*win[overlapskipout[i]];
        overlapskipout[i]++;
      }

      if (counter==hopsize) {
        /*zero output incrementation...*/
        /*last buffer will have gone from 96 to 127...then 2nd last
          will have gone from 64 to 127...*/
        overlapskipout[t] = 0;
        counter = 0;
      }

      outsigl[j] = outsuml;
      outsigr[j] = outsumr;

    }   /* end of ksmps audio loop */

        /*update*/
    p->t = t;
    p->counter = counter;

    return OK;
}

/*see csound manual (extending csound) for details of below*/
OENTRY hrtfopcodes_localops[] = {
  { "hrtfmove",   sizeof(hrtfmove),5, "aa", "akkSSooo",
    (SUBR)hrtfmove_init, NULL, (SUBR)hrtfmove_process },
  { "hrtfstat",   sizeof(hrtfstat),5, "aa", "aiiSSoo",
    (SUBR)hrtfstat_init, NULL, (SUBR)hrtfstat_process },
  { "hrtfmove2",   sizeof(hrtfmove2),5, "aa", "akkSSooo",
    (SUBR)hrtfmove2_init, NULL, (SUBR)hrtfmove2_process },
};

LINKAGE1(hrtfopcodes_localops)
