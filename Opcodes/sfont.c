/*  
    sfont.c:

    Copyright (C) 2000 Gabriel Maldonado, John ffitch

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

/* WARNING! This file MUST be compiled by setting the structure member
   alignment (compiler option) to 1 byte. That is: no padding bytes
   should be present between a structure data member and another.
   This code will cause memory access faults and crash Csound if
   compiled with structure member alignment different than 1. See the
   documentation of your C compiler to choose the appropriate compiler
   directive switch.  */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include "sfenum.h"
#include "sfont.h"
#include "csdl.h"

#define s2d(x)  *((DWORD *) (x))

int chunk_read(FILE *f, CHUNK *chunk);
void fill_SfPointers(ENVIRON *);
void fill_SfStruct(ENVIRON *);
void layerDefaults(layerType *layer);
void splitDefaults(splitType *split);


#define MAX_SFONT               (10)
#define MAX_SFPRESET            (512)
#define MAX_SFINSTR             (512)
#define GLOBAL_ATTENUATION      (FL(0.3))

#define ONETWELTH               (0.08333333333333333333333333333)
#define TWOTOTWELTH             (1.05946309435929526456182529495)

static SFBANK *soundFont;
static SFBANK sfArray[MAX_SFONT];
static int currSFndx = 0;

static presetType *presetp[MAX_SFPRESET];
static SHORT *sampleBase[MAX_SFPRESET];
/* static instrType *instrp[MAX_SFINSTR]; */
/* static SHORT *isampleBase[MAX_SFINSTR]; */
static MYFLT pitches[128];


void SfReset(void)
{
    int j,k,l;
    for (j=0; j<currSFndx; j++) {
      for (k=0; k< sfArray[j].presets_num; k++) {
        for (l=0; l<sfArray[j].preset[k].layers_num; l++) {
          free(sfArray[j].preset[k].layer[l].split);
        }
        free(sfArray[j].preset[k].layer);
      }
      free(sfArray[j].preset);
      for (l=0; l< sfArray[j].instrs_num; l++) {
        free(sfArray[j].instr[l].split);
      }
      free(sfArray[j].instr);
      free(sfArray[j].chunk.main_chunk.ckDATA);
    }
    currSFndx = 0;
}

void fill_pitches(void)
{
    int j;
    for (j=0; j<128; j++) {
      pitches[j] = (MYFLT) (440.0 * pow (2.0,ONETWELTH * (j - 69.0)));
    }
}

void SoundFontLoad(ENVIRON *csound, char *fname)
{
    FILE *fil;
    char pathnam[255];
    printf("\n"
           "******************************************\n"
           "**  Csound SoundFont2 support ver. 1.2  **\n"
           "**          by Gabriel Maldonado        **\n"
           "**        g.maldonado@agora.stm.it      **\n"
           "** http://web.tiscalinet.it/G-Maldonado **\n"
           "******************************************\n\n");
    if (( fil = fopen(fname,"rb")) == NULL) {
#ifdef SSOUND
/**
* Declaring errno causes a problem with the Csound API library,
* which is compiled with multi-threaded code generation.
*/
#else
#if defined(WIN32) && !defined(__CYGWIN__)
	extern volatile int _errno;
	#define errno _errno
#endif
#endif
      char bb[256];
      strcpy(pathnam, fname);  /* in case of error message */
      if (!csound->isfullpath_(fname)) {
        if (csound->ssdirpath_ != NULL) {
          strcpy(pathnam, csound->catpath_(csound->ssdirpath_, fname));
          if ((fil = fopen(pathnam,"rb")) != NULL) goto done;
        }
        if (csound->sfdirpath_ != NULL) {
          strcpy(pathnam, csound->catpath_(csound->sfdirpath_, fname));
          if (( fil = fopen(pathnam,"rb")) != NULL) goto done;
        }
      }
      sprintf(bb,
              csound->getstring_(X_1491,
                  "sfload: cannot open SoundFont file \"%s\" (error %s)"),
              fname, strerror(errno));
      csound->die_(bb);
    }
 done:
    soundFont = &sfArray[currSFndx];
    strcpy(soundFont->name, pathnam);
    chunk_read(fil, &soundFont->chunk.main_chunk);
    fclose(fil);
    fill_pitches();
    fill_SfPointers(csound);
    fill_SfStruct(csound);
}

static int compare(presetType * elem1, presetType *elem2)
{
    if (elem1->bank * 128 + elem1->prog >  elem2->bank * 128 + elem2->prog)
      return 1;
    else
      return -1;
}

/* syntax:
        ihandle SfLoad "filename"
*/

static char *Gfname;

int SfLoad(SFLOAD *p)          /* open a file and return its handle */
{                               /* the handle is simply a stack index */
    char fname[256];
    SFBANK *sf;
    strcpy(fname, unquote(p->STRARG));
    Gfname = fname;
    SoundFontLoad(p->h.insdshead->csound, fname);
    *p->ihandle = (float) currSFndx;
    sf = &sfArray[currSFndx];
    qsort(sf->preset, sf->presets_num, sizeof(presetType),
          (int (*)(const void *, const void * )) compare);
    currSFndx++;
    return OK;
}


char temp_string[24];
char *filter_string(char *s)
{
    int i=0, j=0;
    int c;
    for (i=0; i<22; i++, j++) {
      c = s[j];
      if (c=='\0') break;
      if (isprint(c)) temp_string[i]=c;
      else if (c<32) {
        temp_string[i++]='^';
        temp_string[i] = '@'+c;
      }
      else temp_string[i]='?';
    }
    temp_string[i] = '\0';
    return temp_string;
}

int Sfplist(SFPLIST *p)
{
    SFBANK *sf = &sfArray[(int) *p->ihandle];
    int j;
    printf(Str(X_1492,"\nPreset list of \"%s\"\n"),sf->name);
    for (j =0; j < sf->presets_num; j++) {
      presetType *prs = &sf->preset[j];
      printf(Str(X_1493,"%3d) %-20s\tprog:%-3d bank:%d\n"),
             j, filter_string(prs->name), prs->prog, prs->bank);
    }
    printf("\n");
    return OK;
}


int SfAssignAllPresets(SFPASSIGN *p)
{
    SFBANK *sf = &sfArray[(int) *p->ihandle];
    int pHandle = (int)  *p->startNum, pnum = sf->presets_num;
    int j;
    printf(Str(X_1494,"\nAssigning all Presets of \"%s\" starting from"
               " %d (preset handle number)\n"),
           sf->name, pHandle);
    for (j =0; j < pnum;  j++) {
      presetType *prs = &sf->preset[j];
      printf(Str(X_1495,"%3d<--%-20s\t(prog:%-3d bank:%d)\n"),
             j, prs->name, prs->prog, prs->bank);
      presetp[pHandle] = &sf->preset[j];
      sampleBase[pHandle] = sf->sampleData;
      pHandle++;
    }
    printf(Str(X_1496,
               "\nAll presets have been assigned to preset"
               " handles from %d to %d \n\n"),
           (int) *p->startNum, pHandle-1);
    return OK;
}



int Sfilist(SFPLIST *p)
{
    SFBANK *sf = &sfArray[(int) *p->ihandle];
    int j;
    printf(Str(X_1497,"\nInstrument list of \"%s\"\n"),sf->name);
    for (j =0; j < sf->instrs_num; j++) {
      instrType *inst = &sf->instr[j];
      printf("%3d) %-20s\n", j, inst->name);
    }
    printf("\n");
    return OK;
}


int SfPreset(SFPRESET *p)
{
    int j, presetHandle = (int) *p->iPresetHandle;
    SFBANK *sf = &sfArray[(DWORD) *p->isfhandle];

    if (presetHandle >= MAX_SFPRESET) {
      char s[512];
      sprintf(s,Str(X_1498,"sfpreset: preset handle too big (%d), max: %d"),
              presetHandle, (int) MAX_SFPRESET-1);
      dies("%s",s);
    }

    for (j=0; j< sf->presets_num; j++) {
      if (sf->preset[j].prog == (WORD) *p->iprog &&
          sf->preset[j].bank == (WORD) *p->ibank )
        {
          presetp[presetHandle] = &sf->preset[j];
          sampleBase[presetHandle] = sf->sampleData;
          break;
        }
    }
    *p->ipresethandle = (MYFLT) presetHandle;

    if (presetp[presetHandle] == NULL) {
      char s[512] ;
      sprintf(s,Str(X_1499,"sfpreset: cannot find any preset having prog"
                    ".number %d and bank number %d in SoundFont file \"%s\"\n"),
              (int) *p->iprog ,(int) *p->ibank,
              sfArray[(DWORD) *p->isfhandle].name);
      dies("%s",s);
    }
    return OK;
}


int SfPlay_set(SFPLAY *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset = presetp[index];
    SHORT *sBase = sampleBase[index];
    int layersNum, j, spltNum = 0, flag = (int) *p->iflag;
    if (!preset) {
      return initerror(Str(X_1820,
                           "sfplay: invalid or out-of-range preset number"));
    }
    layersNum = preset->layers_num;
    for (j =0; j < layersNum; j++) {
      layerType *layer = &preset->layer[j];
      int vel= (int) *p->ivel, notnum= (int) *p->inotnum;
      if (notnum >= layer->minNoteRange &&
          notnum <= layer->maxNoteRange &&
          vel    >= layer->minVelRange  &&
          vel    <= layer->maxVelRange) {
        int splitsNum = layer->splits_num, k;
        for (k = 0; k < splitsNum; k++) {
          splitType *split = &layer->split[k];
          if (notnum  >= split->minNoteRange &&
              notnum  <= split->maxNoteRange &&
              vel     >= split->minVelRange  &&
              vel     <= split->maxVelRange) {
            sfSample *sample = split->sample;
            DWORD start=sample->dwStart;
            MYFLT attenuation;
            double pan;
            double freq, orgfreq;
            double tuneCorrection = split->coarseTune + layer->coarseTune +
              (split->fineTune + layer->fineTune)*0.01;
            int orgkey = split->overridingRootKey;
            if (orgkey == -1) orgkey = sample->byOriginalKey;
            orgfreq = pitches[orgkey];
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*sample->dwSampleRate*onedsr;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow(2.0, ONETWELTH * (split->scaleTuning*0.01) * (notnum-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*onedsr;
            }
            attenuation = (MYFLT) (layer->initialAttenuation +
                                   split->initialAttenuation);
            attenuation = (MYFLT) pow(2.0, (-1.0/60.0) * attenuation )
              * GLOBAL_ATTENUATION;
            pan = (double)(split->pan + layer->pan) / 1000.0 + 0.5;
            if (pan > 1.0) pan = 1.0;
            else if (pan < 0.0) pan = 0.0;
            /* Suggested fix from steven yi Oct 2002 */
            p->base[spltNum] = sBase + start;
            p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
            p->end[spltNum] = sample->dwEnd + split->endOffset - start;
            p->startloop[spltNum] =
              sample->dwStartloop + split->startLoopOffset  - start;
            p->endloop[spltNum] =
              sample->dwEndloop + split->endLoopOffset - start;
            p->leftlevel[spltNum] = (MYFLT) sqrt(1.0-pan) * attenuation;
            p->rightlevel[spltNum] = (MYFLT) sqrt(pan) * attenuation;
            p->mode[spltNum]= split->sampleModes;
            spltNum++;
          }
        }
      }
    }
    p->spltNum = spltNum;
    return OK;
}


#define Linear_interpolation \
        SHORT *curr_samp = *base + (long) *phs;\
        MYFLT fract = (MYFLT) *phs - (MYFLT)((long)*phs);\
        MYFLT out = (*curr_samp + (*(curr_samp+1) - *curr_samp)*fract);

#define Cubic_interpolation \
        MYFLT phs1 = (MYFLT) *phs -FL(1.0);\
        int   x0 = (long)phs1 ;\
        MYFLT fract = (MYFLT)(phs1 - x0);\
        SHORT *ftab = *base + x0;\
        MYFLT ym1= *ftab++;\
        MYFLT y0 = *ftab++;\
        MYFLT y1 = *ftab++;\
        MYFLT y2 = *ftab;\
        MYFLT frsq = fract*fract;\
        MYFLT frcu = frsq*ym1;\
        MYFLT t1   = y2 + FL(3.0)*y0;\
        MYFLT out =  y0 + FL(0.5)*frcu + \
                fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) + \
                frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)* y1 - y0);

#define Looped \
        if (*phs >= *startloop) flag = 1;                 \
        if (flag) {                                       \
          while (*phs >= *endloop) *phs -= looplength;    \
          while (*phs < *startloop) *phs += looplength;   \
        }

#define Unlooped \
        if (*phs > *end) break;           \
        if (*phs < FL(0.0)) *phs = FL(0.0);       \


#define Mono_out \
        *outemp1++ +=  *attenuation * out; \
        *phs += si;


#define Stereo_out \
        *outemp1++ += *left * out;\
        *outemp2++ += *right * out;\
        *phs += si;



int SfPlay(SFPLAY *p)
{
    MYFLT *out1 = p->out1, *out2 = p->out2;
    int   nsmps = ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel;
    MYFLT *outemp1 = out1, *outemp2 = out2;

    arate = (p->XINCODE) ? 1 : 0;
    do {
      *outemp1++ = FL(0.0);
      *outemp2++ = FL(0.0);
    } while(--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;
        nsmps = ksmps;
        outemp1 = out1;
        outemp2 = out2;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Stereo_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Stereo_out  Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;  outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Linear_interpolation Stereo_out     Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Linear_interpolation Stereo_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }
    outemp1 = out1;  outemp2 = out2;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do {
        *outemp1++ *= *amp;
        *outemp2++ *= *amp++;
      } while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do {
        *outemp1++ *= famp;
        *outemp2++ *= famp;
      } while (--nsmps);
    }
    return OK;
}


int SfPlay3(SFPLAY *p)
{
    MYFLT *out1 = p->out1, *out2 = p->out2;
    int nsmps = ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop = p->startloop, *endloop = p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel;
    MYFLT *outemp1 = out1, *outemp2 = out2;
    arate = (p->XINCODE) ? 1 : 0;

    do {
      *outemp1++ = FL(0.0);
      *outemp2++ = FL(0.0);
    } while(--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;
        nsmps = ksmps;
        outemp1 = out1; outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Stereo_out      Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while(j--) {
        double looplength = *endloop - *startloop, si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1; outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Cubic_interpolation Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Cubic_interpolation Stereo_out      Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }

    outemp1 = out1;
    outemp2 = out2;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do {
        *outemp1++ *= *amp;
        *outemp2++ *= *amp++;
      } while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do {
        *outemp1++ *= famp;
        *outemp2++ *= famp;
      } while (--nsmps);
    }
    return OK;
}

int SfPlayMono_set(SFPLAYMONO *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset = presetp[index];
    SHORT *sBase = sampleBase[index];
    /* int layersNum= preset->layers_num, j, spltNum = 0, flag=(int) *p->iflag; */

    int layersNum, j, spltNum = 0, flag=(int) *p->iflag;
    if (!preset) {
      return initerror(Str(X_1823,
                           "sfplaym: invalid or out-of-range preset number"));
    }
    layersNum= preset->layers_num;
    for (j =0; j < layersNum; j++) {
      layerType *layer = &preset->layer[j];
      int vel= (int) *p->ivel, notnum= (int) *p->inotnum;
      if (notnum >= layer->minNoteRange &&
          notnum <= layer->maxNoteRange &&
          vel >= layer->minVelRange  &&
          vel <= layer->maxVelRange) {
        int splitsNum = layer->splits_num, k;
        for (k = 0; k < splitsNum; k++) {
          splitType *split = &layer->split[k];
          if (notnum >= split->minNoteRange &&
              notnum <= split->maxNoteRange &&
              vel >= split->minVelRange  &&
              vel <= split->maxVelRange) {
            sfSample *sample = split->sample;
            DWORD start=sample->dwStart;
            double freq, orgfreq;
            double tuneCorrection = split->coarseTune + layer->coarseTune +
              (split->fineTune + layer->fineTune)*0.01;
            int orgkey = split->overridingRootKey;
            if (orgkey == -1) orgkey = sample->byOriginalKey;
            orgfreq = pitches[orgkey] ;
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*sample->dwSampleRate*onedsr;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (notnum-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*onedsr;
            }
            p->attenuation[spltNum] =
              (MYFLT) pow(2.0, (-1.0/60.0) * (layer->initialAttenuation +
                                              split->initialAttenuation)) *
              GLOBAL_ATTENUATION;
            p->base[spltNum] =  sBase+ start;
            p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
            p->end[spltNum] = sample->dwEnd + split->endOffset - start;
            p->startloop[spltNum] = sample->dwStartloop +
              split->startLoopOffset - start;
            p->endloop[spltNum] = sample->dwEndloop + split->endLoopOffset - start;
            p->mode[spltNum]= split->sampleModes;
            spltNum++;
          }
        }
      }
    }
    p->spltNum = spltNum;
    return OK;
}


int SfPlayMono(SFPLAYMONO *p)
{
    MYFLT *out1 = p->out1  ;
    int nsmps = ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end= p->end, *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation;
    MYFLT *outemp1 = out1 ;

    arate = (p->XINCODE) ? 1 : 0;

    do {
      *outemp1++ = FL(0.0);
    } while(--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Linear_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Linear_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    outemp1 = out1;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do        *outemp1++ *= *amp++;
      while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do        *outemp1++ *= famp;
      while (--nsmps);
    }
    return OK;
}

int SfPlayMono3(SFPLAYMONO *p)
{
    MYFLT *out1 = p->out1;
    int nsmps = ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop = p->startloop, *endloop = p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation;
    MYFLT *outemp1 = out1 ;

    arate = (p->XINCODE) ? 1 : 0;

    do  *outemp1++ = FL(0.0);
    while(--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;
        nsmps = ksmps;
        outemp1 = out1;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Mono_out        Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Mono_out        Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Cubic_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Cubic_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    outemp1 = out1;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do        *outemp1++ *= *amp++;
      while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do        *outemp1++ *= famp;
      while (--nsmps);
    }

    return OK;
}

int SfInstrPlay_set(SFIPLAY *p)
{
    int index = (int) *p->sfBank;
    SFBANK *sf = &sfArray[index];
    if (index > currSFndx || *p->instrNum >  sf->instrs_num) {
      return initerror(Str(X_1821,"sfinstr: instrument out of range"));
    }
    else {
      instrType *layer = &sf->instr[(int) *p->instrNum];
      SHORT *sBase = sf->sampleData;
      int spltNum = 0, flag=(int) *p->iflag;
      int vel= (int) *p->ivel, notnum= (int) *p->inotnum;
      int splitsNum = layer->splits_num, k;
      for (k = 0; k < splitsNum; k++) {
        splitType *split = &layer->split[k];
        if (notnum >= split->minNoteRange &&
            notnum <= split->maxNoteRange &&
            vel >= split->minVelRange  &&
            vel <= split->maxVelRange) {
          sfSample *sample = split->sample;
          DWORD start=sample->dwStart;
          MYFLT attenuation, pan;
          double freq, orgfreq;
          double tuneCorrection = split->coarseTune + split->fineTune*0.01;
          int orgkey = split->overridingRootKey;
          if (orgkey == -1) orgkey = sample->byOriginalKey;
          orgfreq = pitches[orgkey] ;
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*sample->dwSampleRate*onedsr;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01)*(notnum - orgkey));
            p->si[spltNum] = (freq/orgfreq) * (sample->dwSampleRate*onedsr);
          }
          attenuation = (MYFLT) (split->initialAttenuation);
          attenuation = (MYFLT) pow(2.0, (-1.0/60.0) * attenuation) *
            GLOBAL_ATTENUATION;
          pan = (MYFLT)  split->pan / FL(1000.0) + FL(0.5);
          if (pan > FL(1.0)) pan =FL(1.0);
          else if (pan < FL(0.0)) pan = FL(0.0);
          p->base[spltNum] = sBase + start;
          p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
          p->end[spltNum] = sample->dwEnd + split->endOffset - start;
          p->startloop[spltNum] = sample->dwStartloop +
            split->startLoopOffset - start;
          p->endloop[spltNum] = sample->dwEndloop + split->endLoopOffset - start;
          p->leftlevel[spltNum] = (FL(1.0)-pan) * attenuation;
          p->rightlevel[spltNum] = pan * attenuation;
          p->mode[spltNum]= split->sampleModes;
          spltNum++;
        }
      }
      p->spltNum = spltNum;
    }
    return OK;
}

int SfInstrPlay(SFIPLAY *p)
{
    MYFLT *out1= p->out1, *out2= p->out2;
    int nsmps= ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel;
    MYFLT *outemp1 = out1, *outemp2 = out2;

    arate = (p->XINCODE) ? 1 : 0;

    do {
      *outemp1++ = FL(0.0);
      *outemp2++ = FL(0.0);
    } while (--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        nsmps = ksmps;
        outemp1 = out1;
        outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation        Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Stereo_out     Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Linear_interpolation        Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Linear_interpolation        Stereo_out      Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }

    outemp1 = out1;
    outemp2 = out2;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do {
        *outemp1++ *= *amp;
        *outemp2++ *= *amp++;
      } while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do {
        *outemp1++ *= famp;
        *outemp2++ *= famp;
      } while (--nsmps);
    }
    return OK;
}

int SfInstrPlay3(SFIPLAY *p)
{
    MYFLT *out1= p->out1, *out2= p->out2;
    int nsmps= ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel;
    MYFLT *outemp1 = out1, *outemp2 = out2;

    arate = (p->XINCODE) ? 1 : 0;

    do {
      *outemp1++ = FL(0.0);
      *outemp2++ = FL(0.0);
    } while (--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        nsmps = ksmps;
        outemp1 = out1;
        outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Stereo_out      Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        outemp2 = out2;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Cubic_interpolation Stereo_out      Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Cubic_interpolation Stereo_out      Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++;
      }
    }

    outemp1 = out1;
    outemp2 = out2;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do {
        *outemp1++ *= *amp;
        *outemp2++ *= *amp++;
      } while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do {
        *outemp1++ *= famp;
        *outemp2++ *= famp;
      } while (--nsmps);
    }
    return OK;
}



int SfInstrPlayMono_set(SFIPLAYMONO *p)
{
    int index = (int) *p->sfBank;
    SFBANK *sf = &sfArray[index];
    if (index > currSFndx || *p->instrNum >  sf->instrs_num) {
      return initerror(Str(X_1500,"sfinstr: instrument out of range"));
    }
    else {
      instrType *layer = &sf->instr[(int) *p->instrNum];
      SHORT *sBase = sf->sampleData;
      int spltNum = 0, flag=(int) *p->iflag;
      int vel= (int) *p->ivel, notnum= (int) *p->inotnum;
      int splitsNum = layer->splits_num, k;
      for (k = 0; k < splitsNum; k++) {
        splitType *split = &layer->split[k];
        if (notnum >= split->minNoteRange &&
            notnum <= split->maxNoteRange &&
            vel >= split->minVelRange  &&
            vel     <= split->maxVelRange) {
          sfSample *sample = split->sample;
          DWORD start=sample->dwStart;
          double freq, orgfreq;
          double tuneCorrection = split->coarseTune + split->fineTune/100.0;
          int orgkey = split->overridingRootKey;
          if (orgkey == -1) orgkey = sample->byOriginalKey;
          orgfreq = pitches[orgkey];
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*sample->dwSampleRate*onedsr;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (notnum-orgkey));
            p->si[spltNum] = (freq/orgfreq) * (sample->dwSampleRate*onedsr);
          }
          p->attenuation[spltNum] = (MYFLT) pow(2.0, (-1.0/60.0)*
                                                split->initialAttenuation)
            * GLOBAL_ATTENUATION;
          p->base[spltNum] = sBase+ start;
          p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
          p->end[spltNum] = sample->dwEnd + split->endOffset - start;
          p->startloop[spltNum] = sample->dwStartloop +
            split->startLoopOffset - start;
          p->endloop[spltNum] = sample->dwEndloop + split->endLoopOffset - start;
          p->mode[spltNum]= split->sampleModes;
          spltNum++;
        }
      }
      p->spltNum = spltNum;
    }
    return OK;
}

int SfInstrPlayMono(SFIPLAYMONO *p)
{
    MYFLT *out1= p->out1  ;
    int nsmps= ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;

    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation;
    MYFLT *outemp1 = out1 ;

    arate = (p->XINCODE) ? 1 : 0;

    do  *outemp1++ = FL(0.0);
    while (--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation        Mono_out        Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Linear_interpolation Mono_out       Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Linear_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Linear_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    outemp1 = out1;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do        *outemp1++ *= *amp;
      while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do        *outemp1++ *= famp;
      while (--nsmps);
    }
    return OK;
}


int SfInstrPlayMono3(SFIPLAYMONO *p)
{
    MYFLT *out1= p->out1  ;
    int nsmps= ksmps, j = p->spltNum, arate;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation;
    MYFLT *outemp1 = out1 ;

    arate = (p->XINCODE) ? 1 : 0;

    do  *outemp1++ = FL(0.0);
    while (--nsmps);

    if (arate) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            double si = *sampinc * *freq++;
            Cubic_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        nsmps = ksmps;
        outemp1 = out1;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
          do {
            Cubic_interpolation Mono_out Looped
          } while (--nsmps);
        }
        else if (*phs < *end) {
          do {
            Cubic_interpolation Mono_out Unlooped
          } while (--nsmps);
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++;
      }
    }
    outemp1 = out1;
    nsmps = ksmps;
    if (arate) {
      MYFLT *amp = p->xamp;
      do        *outemp1++ *= *amp++;
      while (--nsmps);
    }
    else {
      MYFLT famp = *p->xamp;
      do        *outemp1++ *= famp;
      while (--nsmps);
    }
    return OK;
}

/*********************/

/*  Convert Big-endian <-> Little-endian (for Big-endian machines only)
 *  fmt: ((b|w|d)[0-9]*)+
 *  b:byte (no conversion), w:word, d:double word, digits(optional):repeat n times
 */
#ifdef WORDS_BIGENDIAN
static void
ChangeByteOrder(char *fmt, char *p, long size)
{
    char c, c1, c2, c3, c4;
    char *fmt_org = fmt;
    long i, times;
    while (size > 0) {
      fmt = fmt_org;
      while (*fmt) {
        c = *fmt++;
        if (isdigit(*fmt)) {
          times = strtol(fmt, &fmt, 0);
        } else {
          times = 1;
        }
        for (i = 0; i < times; i++) {
          switch(c) {
          case 'b': case 'B':
            p++; size--; break;
          case 'w': case 'W':
            c1 = p[0]; c2 = p[1];
            *p++ = c2; *p++ = c1;
            size -= 2; break;
          case 'd': case 'D':
            c1 = p[0]; c2 = p[1]; c3 = p[2]; c4 = p[3];
            *p++ = c4; *p++ = c3; *p++ = c2; *p++ = c1;
            size -= 4;
            break;
          }
        }
      }
    }
}
#else
#define ChangeByteOrder(fmt, p, size) /* nothing */
#endif

void fill_SfStruct(ENVIRON *csound)
{
#if defined(SYMANTEC)
#pragma options(!global_optimizer)
#endif
    int j, k, i, l, m, size, iStart, iEnd, kk, ll, mStart, mEnd;
    int pbag_num,first_pbag,layer_num;
    int ibag_num,first_ibag,split_num;
    CHUNK *phdrChunk= soundFont->chunk.phdrChunk;
    presetType *preset;
    sfPresetHeader *phdr = soundFont->chunk.phdr;
    sfPresetBag *pbag = soundFont->chunk.pbag;
    sfGenList *pgen = soundFont->chunk.pgen;
    sfInst *inst = soundFont->chunk.inst;
    sfInstBag *ibag = soundFont->chunk.ibag;
/*     sfInstModList *imod = soundFont->chunk.imod; */
    sfInstGenList *igen = soundFont->chunk.igen;
    sfSample *shdr = soundFont->chunk.shdr;

    size = phdrChunk->ckSize / sizeof(sfPresetHeader);
    soundFont->presets_num = size;
    preset = (presetType *) malloc(size * sizeof(sfPresetHeader));
    for (j=0; j < size; j++) {
      preset[j].name = phdr[j].achPresetName;
      if (strcmp(preset[j].name,"EOP")==0) {
        soundFont->presets_num = j;
        goto end_fill_presets;
      }
      preset[j].num = j;
      preset[j].prog = phdr[j].wPreset;
      preset[j].bank = phdr[j].wBank;
      first_pbag = phdr[j].wPresetBagNdx;
      pbag_num = phdr[j+1].wPresetBagNdx - first_pbag;
      layer_num = 0;
      for  (k = 0 ; k < pbag_num ; k++) {
        iStart = pbag[k+first_pbag].wGenNdx;
        iEnd = pbag[k+first_pbag+1].wGenNdx;
        for (i = iStart; i < iEnd; i++) {
          if (pgen[i].sfGenOper == instrument ) {
            layer_num++;
          }
        }
      }
      preset[j].layers_num=layer_num;
      preset[j].layer = (layerType *) malloc ( layer_num * sizeof(layerType));
      for (k=0; k <layer_num; k++) {
        layerDefaults(&preset[j].layer[k]);
      }
      for  (k = 0, kk=0; k < pbag_num ; k++) {
        iStart = pbag[k+first_pbag].wGenNdx;
        iEnd = pbag[k+first_pbag+1].wGenNdx;
        for (i = iStart; i < iEnd; i++) {
          layerType *layer;
          layer = &preset[j].layer[kk];
          switch (pgen[i].sfGenOper) {
          case instrument:
            {
#define UNUSE 0x7fffffff
              int GsampleModes=UNUSE, GcoarseTune=UNUSE, GfineTune=UNUSE;
              int Gpan=UNUSE, GinitialAttenuation=UNUSE,GscaleTuning=UNUSE;
              int GoverridingRootKey = UNUSE;

              layer->num  = pgen[i].genAmount.wAmount;
              layer->name = inst[layer->num].achInstName;
              first_ibag = inst[layer->num].wInstBagNdx;
              ibag_num = inst[layer->num +1].wInstBagNdx - first_ibag;
              split_num = 0;
              for (l=0; l < ibag_num; l++) {
                mStart =        ibag[l+first_ibag].wInstGenNdx;
                mEnd = ibag[l+first_ibag+1].wInstGenNdx;
                for (m=mStart; m < mEnd; m++) {
                  if (igen[m].sfGenOper == sampleID) {
                    split_num++;
                  }
                }
              }
              layer->splits_num = split_num;
              layer->split = (splitType *) malloc ( split_num * sizeof(splitType));
              for (l=0; l<split_num; l++) {
                splitDefaults(&layer->split[l]);
              }
              for (l=0, ll=0; l < ibag_num; l++) {
                int sglobal_zone = 1;
                mStart = ibag[l+first_ibag].wInstGenNdx;
                mEnd = ibag[l+first_ibag+1].wInstGenNdx;

                for (m=mStart; m < mEnd; m++) {
                  if (igen[m].sfGenOper == sampleID) sglobal_zone=0;
                }
                if (sglobal_zone) {
                  for (m=mStart; m < mEnd; m++) {
                    switch (igen[m].sfGenOper) {
                    case sampleID:
                      break;
                    case overridingRootKey:
                      GoverridingRootKey = igen[m].genAmount.wAmount;
                      break;
                    case coarseTune:
                      GcoarseTune =  igen[m].genAmount.shAmount;
                      break;
                    case fineTune:
                      GfineTune = igen[m].genAmount.shAmount;
                      break;
                    case scaleTuning:
                      GscaleTuning = igen[m].genAmount.shAmount;
                      break;
                    case pan:
                      Gpan = igen[m].genAmount.shAmount;
                      break;
                    case sampleModes:
                      GsampleModes =  igen[m].genAmount.wAmount;
                      break;
                    case initialAttenuation:
                      GinitialAttenuation = igen[m].genAmount.shAmount;
                      break;
                    case keyRange:
                      break;
                    case velRange:
                      break;
                    }
                  }
                }
                else {
                  splitType *split;
                  split = &layer->split[ll];
                  if (GoverridingRootKey != UNUSE)
                    split->overridingRootKey = (BYTE) GoverridingRootKey;
                  if (GcoarseTune != UNUSE)
                    split->coarseTune = (BYTE) GcoarseTune;
                  if (GfineTune != UNUSE)
                    split->fineTune = (BYTE) GfineTune;
                  if (GscaleTuning != UNUSE)
                    split->scaleTuning = (BYTE) GscaleTuning;
                  if (Gpan != UNUSE)
                    split->pan = (BYTE) Gpan;
                  if (GsampleModes != UNUSE)
                    split->sampleModes = (BYTE) GsampleModes;
                  if (GinitialAttenuation != UNUSE)
                    split->initialAttenuation = (BYTE) GinitialAttenuation;

                  for (m=mStart; m < mEnd; m++) {
                    switch (igen[m].sfGenOper) {
                    case sampleID:
                      {
                        int num = igen[m].genAmount.wAmount;
                        split->num= num;
                        split->sample = &shdr[num];
                        if (split->sample->sfSampleType & 0x8000) {
                          char buf[256];
                          sprintf(buf,"SoundFont file \"%s\" contains ROM samples!"
                                  "\nAt present time only RAM samples are allowed"
                                  " by sfload. \nSession aborted!",Gfname);
                          csound->die_(buf);
                        }
                        sglobal_zone = 0;
                        ll++;
                      }
                      break;
                    case overridingRootKey:
                      split->overridingRootKey = (BYTE) igen[m].genAmount.wAmount;
                      break;
                    case coarseTune:
                      split->coarseTune = (char) igen[m].genAmount.shAmount;
                      break;
                    case fineTune:
                      split->fineTune = (char) igen[m].genAmount.shAmount;
                      break;
                    case scaleTuning:
                      split->scaleTuning = igen[m].genAmount.shAmount;
                      break;
                    case pan:
                      split->pan = igen[m].genAmount.shAmount;
                      break;
                    case sampleModes:
                      split->sampleModes = (BYTE) igen[m].genAmount.wAmount;
                      break;
                    case initialAttenuation:
                      split->initialAttenuation = igen[m].genAmount.shAmount;
                      break;
                    case keyRange:
                      split->minNoteRange = igen[m].genAmount.ranges.byLo;
                      split->maxNoteRange = igen[m].genAmount.ranges.byHi;
                      break;
                    case velRange:
                      split->minVelRange = igen[m].genAmount.ranges.byLo;
                      split->maxVelRange = igen[m].genAmount.ranges.byHi;
                      break;
                    case startAddrsOffset:
                      split->startOffset += igen[m].genAmount.shAmount;
                      break;
                    case endAddrsOffset:
                      split->endOffset += igen[m].genAmount.shAmount;
                      break;
                    case startloopAddrsOffset:
                      split->startLoopOffset += igen[m].genAmount.shAmount;
                      break;
                    case endloopAddrsOffset:
                      split->endLoopOffset += igen[m].genAmount.shAmount;
                      break;
                    case startAddrsCoarseOffset:
                      split->startOffset += igen[m].genAmount.shAmount * 32768;
                      break;
                    case endAddrsCoarseOffset:
                      split->endOffset += igen[m].genAmount.shAmount * 32768;
                      break;
                    case startloopAddrCoarseOffset:
                      split->startLoopOffset += igen[m].genAmount.shAmount * 32768;
                      break;
                    case endloopAddrsCoarseOffset:
                      split->endLoopOffset += igen[m].genAmount.shAmount * 32768;
                      break;
                    case keynum:
                      /*printf("");*/
                      break;
                    case velocity:
                      /*printf("");*/
                      break;
                    case exclusiveClass:
                      /*printf("");*/
                      break;

                    }
                  }
                }
              }
              kk++;
            }
            break;
          case coarseTune:
            layer->coarseTune = (char) pgen[i].genAmount.shAmount;
            break;
          case fineTune:
            layer->fineTune = (char) pgen[i].genAmount.shAmount;
            break;
          case scaleTuning:
            layer->scaleTuning = pgen[i].genAmount.shAmount;
            break;
          case initialAttenuation:
            layer->initialAttenuation = pgen[i].genAmount.shAmount;
            break;
          case pan:
            layer->pan = pgen[i].genAmount.shAmount;
            break;
          case keyRange:
            layer->minNoteRange = pgen[i].genAmount.ranges.byLo;
            layer->maxNoteRange = pgen[i].genAmount.ranges.byHi;
            break;
          case velRange:
            layer->minVelRange = pgen[i].genAmount.ranges.byLo;
            layer->maxVelRange = pgen[i].genAmount.ranges.byHi;
            break;
          }
        }
      }
    }
 end_fill_presets:
    soundFont->preset = preset;
/* fill layer list */
    {
      instrType *instru;
      size = soundFont->chunk.instChunk->ckSize / sizeof(sfInst);
      soundFont->instrs_num = size;
      instru = (instrType *) malloc(size * sizeof(layerType));
      for (j=0; j < size; j++) {
#define UNUSE 0x7fffffff
        int GsampleModes=UNUSE, GcoarseTune=UNUSE, GfineTune=UNUSE;
        int Gpan=UNUSE, GinitialAttenuation=UNUSE,GscaleTuning=UNUSE;
        int GoverridingRootKey = UNUSE;

        instru[j].name = inst[j].achInstName;
        if (strcmp(instru[j].name,"EOI")==0) {
          soundFont->instrs_num = j;
          goto end_fill_layers;
        }
        instru[j].num = j;
        first_ibag = inst[j].wInstBagNdx;
        ibag_num = inst[j+1].wInstBagNdx - first_ibag;
        split_num=0;
        for (l=0; l < ibag_num; l++) {
          mStart =      ibag[l+first_ibag].wInstGenNdx;
          mEnd = ibag[l+first_ibag+1].wInstGenNdx;
          for (m=mStart; m < mEnd; m++) {
            if (igen[m].sfGenOper == sampleID) {
              split_num++;
            }
          }
        }
        instru[j].splits_num = split_num;
        instru[j].split = (splitType *) malloc ( split_num * sizeof(splitType));
        for (l=0; l<split_num; l++) {
          splitDefaults(&instru[j].split[l]);
        }
        for (l=0, ll=0; l < ibag_num; l++) {
          int sglobal_zone = 1;
          mStart = ibag[l+first_ibag].wInstGenNdx;
          mEnd = ibag[l+first_ibag+1].wInstGenNdx;

          for (m=mStart; m < mEnd; m++) {
            if (igen[m].sfGenOper == sampleID) sglobal_zone=0;
          }
          if (sglobal_zone) {
            for (m=mStart; m < mEnd; m++) {
              switch (igen[m].sfGenOper) {
              case sampleID:
                break;
              case overridingRootKey:
                GoverridingRootKey = igen[m].genAmount.wAmount;
                break;
              case coarseTune:
                GcoarseTune =  igen[m].genAmount.shAmount;
                break;
              case fineTune:
                GfineTune = igen[m].genAmount.shAmount;
                break;
              case scaleTuning:
                GscaleTuning = igen[m].genAmount.shAmount;
                break;
              case pan:
                Gpan = igen[m].genAmount.shAmount;
                break;
              case sampleModes:
                GsampleModes =  igen[m].genAmount.wAmount;
                break;
              case initialAttenuation:
                GinitialAttenuation = igen[m].genAmount.shAmount;
                break;
              case keyRange:
                break;
              case velRange:
                break;
              }
            }
          }
          else {
            splitType *split;
            split = &instru[j].split[ll];
            if (GoverridingRootKey != UNUSE)
              split->overridingRootKey = (BYTE) GoverridingRootKey;
            if (GcoarseTune != UNUSE)
              split->coarseTune = (BYTE) GcoarseTune;
            if (GfineTune != UNUSE)
              split->fineTune = (BYTE) GfineTune;
            if (GscaleTuning != UNUSE)
              split->scaleTuning = (BYTE) GscaleTuning;
            if (Gpan != UNUSE)
              split->pan = (BYTE) Gpan;
            if (GsampleModes != UNUSE)
              split->sampleModes = (BYTE) GsampleModes;
            if (GinitialAttenuation != UNUSE)
              split->initialAttenuation = (BYTE) GinitialAttenuation;

            for (m=mStart; m < mEnd; m++) {
              switch (igen[m].sfGenOper) {
              case sampleID:
                {
                  int num = igen[m].genAmount.wAmount;
                  split->num= num;
                  split->sample = &shdr[num];
                  if (split->sample->sfSampleType & 0x8000) {
                    char buf[256];
                    sprintf(buf,"SoundFont file \"%s\" contains ROM samples! "
                            "\nAt present time only RAM samples are allowed "
                            "by sfload. \nSession aborted!",Gfname);
                    csound->die_(buf);
                  }
                  sglobal_zone = 0;
                  ll++;
                }
                break;
              case overridingRootKey:
                split->overridingRootKey = (BYTE) igen[m].genAmount.wAmount;
                break;
              case coarseTune:
                split->coarseTune = (char) igen[m].genAmount.shAmount;
                break;
              case fineTune:
                split->fineTune = (char) igen[m].genAmount.shAmount;
                break;
              case scaleTuning:
                split->scaleTuning = igen[m].genAmount.shAmount;
                break;
              case pan:
                split->pan = igen[m].genAmount.shAmount;
                break;
              case sampleModes:
                split->sampleModes = (BYTE) igen[m].genAmount.wAmount;
                break;
              case initialAttenuation:
                split->initialAttenuation = igen[m].genAmount.shAmount;
                break;
              case keyRange:
                split->minNoteRange = igen[m].genAmount.ranges.byLo;
                split->maxNoteRange = igen[m].genAmount.ranges.byHi;
                break;
              case velRange:
                split->minVelRange = igen[m].genAmount.ranges.byLo;
                split->maxVelRange = igen[m].genAmount.ranges.byHi;
                break;
              case startAddrsOffset:
                split->startOffset += igen[m].genAmount.shAmount;
                break;
              case endAddrsOffset:
                split->endOffset += igen[m].genAmount.shAmount;
                break;
              case startloopAddrsOffset:
                split->startLoopOffset += igen[m].genAmount.shAmount;
                break;
              case endloopAddrsOffset:
                split->endLoopOffset += igen[m].genAmount.shAmount;
                break;
              case startAddrsCoarseOffset:
                split->startOffset += igen[m].genAmount.shAmount * 32768;
                break;
              case endAddrsCoarseOffset:
                split->endOffset += igen[m].genAmount.shAmount * 32768;
                break;
              case startloopAddrCoarseOffset:
                split->startLoopOffset += igen[m].genAmount.shAmount * 32768;
                break;
              case endloopAddrsCoarseOffset:
                split->endLoopOffset += igen[m].genAmount.shAmount * 32768;
                break;
              case keynum:
                /*printf("");*/
                break;
              case velocity:
                /*printf("");*/
                break;
              case exclusiveClass:
                /*printf("");*/
                break;
              }
            }
          }
        }
      }
    end_fill_layers:
      soundFont->instr = instru;
    }
}

void layerDefaults(layerType *layer)
{
    layer->splits_num         = 0;
    layer->minNoteRange       = 0;
    layer->maxNoteRange       = 127;
    layer->minVelRange        = 0;
    layer->maxVelRange        = 127;
    layer->coarseTune         = 0;
    layer->fineTune           = 0;
    layer->scaleTuning        = 0;
    layer->initialAttenuation = 0;
    layer->pan                = 0;
}

void splitDefaults(splitType *split)
{
    split->sampleModes        = 0;
    split->minNoteRange       = 0;
    split->maxNoteRange       = 127;
    split->minVelRange        = 0;
    split->maxVelRange        = 127;
    split->startOffset        = 0;
    split->endOffset          = 0;
    split->startLoopOffset    = 0;
    split->endLoopOffset      = 0;
    split->overridingRootKey  = -1;
    split->coarseTune         = 0;
    split->fineTune           = 0;
    split->scaleTuning        = 100;
    split->initialAttenuation = 0;
    split->pan                = 0;
}

int chunk_read(FILE *fil, CHUNK *chunk)
{
    fread(chunk->ckID,1,4, fil);
    fread(&chunk->ckSize,4,1,fil);
    ChangeByteOrder("d", (char *)&chunk->ckSize, 4);
    chunk->ckDATA = (BYTE *) malloc( chunk->ckSize);
#ifdef BETA
    printf("read chunk (%.4s) length %ld\n", (char*)&chunk->ckID, chunk->ckSize);
#endif
    return fread(chunk->ckDATA,1,chunk->ckSize,fil);
}

DWORD dword(char *p)
{
    union cheat {
      DWORD i;
      char c[4];
    } x;
    x.c[0] = *p++;
    x.c[1] = *p++;
    x.c[2] = *p++;
    x.c[3] = *p++;
    return x.i;
}

void fill_SfPointers(ENVIRON *csound)
{
    char *chkp;
    DWORD chkid, j, size;
    CHUNK *main_chunk=&soundFont->chunk.main_chunk;
    CHUNK *smplChunk=NULL, *phdrChunk=NULL, *pbagChunk=NULL, *pmodChunk=NULL;
    CHUNK *pgenChunk=NULL, *instChunk=NULL, *ibagChunk=NULL, *imodChunk=NULL;
    CHUNK *igenChunk=NULL, *shdrChunk=NULL;
    if (main_chunk->ckDATA == NULL) {
      csound->die_(csound->getstring_(X_1555, "Sfont format not compatible"));
    }
    chkp = (char *) main_chunk->ckDATA+4;
    for  (j=4; j< main_chunk->ckSize;) {
      chkid = /* (DWORD *) chkp*/ dword(chkp);
#ifdef BETA
      printf("Looking at %.4s\n", (char*)&chkid);
#endif
      if (chkid == s2d("LIST")) {
#ifdef BETA
        printf("LIST ");
#endif
        j += 4; chkp += 4;
        ChangeByteOrder("d", chkp, 4);
        size = /* (DWORD *) chkp */ dword(chkp);
#ifdef BETA
        printf("**size %ld %ld\n", size, *((DWORD *) chkp));
#endif
        j += 4; chkp += 4;
        chkid = /* (DWORD *) chkp */ dword(chkp);
#ifdef BETA
        printf("**chkid %p %p\n", (void*)chkid, (void*)(*((DWORD *) chkp)));
        printf(":Looking at %.4s (%ld)\n", (char*)&chkid, size);
#endif
        if (chkid == s2d("INFO")) {
#ifdef BETA
          printf("INFO ");
#endif
          chkp += size;
          j    += size;
        }
        else if (chkid == s2d("sdta")) {
#ifdef BETA
          printf("sdta ");
#endif
          j +=4; chkp += 4;
          smplChunk = (CHUNK *) chkp;
          soundFont->sampleData = (SHORT *) &smplChunk->ckDATA;
#ifdef BETA
          printf("Change %d and then %ld times w\n", *(chkp + 4), size - 12);
#endif
          ChangeByteOrder("d", chkp + 4, 4);
          ChangeByteOrder("w", chkp + 8, size - 12);
#ifdef BETA
          {
            DWORD i;
            for (i=size-12; i< size+4; i++) printf("%c(%.2x)",chkp[i], chkp[i]);
            printf("\n");
          }
#endif
          chkp += size-4;
          j += size-4;
        }
        else if (chkid  ==  s2d("pdta")) {
#ifdef BETA
          printf("pdta ");
#endif
          j += 4; chkp += 4;
          do {
            chkid = /* (DWORD *) chkp */ dword(chkp);
            /*            printf("::Looking at %.4s (%d)\n", &chkid, size); */
            if (chkid == s2d("phdr")) {
#ifdef BETA
              printf("phdr ");
#endif
              phdrChunk = (CHUNK *) chkp;
              soundFont->chunk.phdr= (sfPresetHeader *) &phdrChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20w3d3", chkp + 8, phdrChunk->ckSize);
              chkp += phdrChunk->ckSize+8;
              j += phdrChunk->ckSize+8;
            }
            else if (chkid == s2d("pbag")) {
#ifdef BETA
              printf("pbag ");
#endif
              pbagChunk = (CHUNK *) chkp;
              soundFont->chunk.pbag= (sfPresetBag *) &pbagChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, pbagChunk->ckSize);
              chkp += pbagChunk->ckSize+8;
              j += pbagChunk->ckSize+8;
            }
            else if (chkid == s2d("pmod")) {
#ifdef BETA
              printf("pmod ");
#endif
              pmodChunk = (CHUNK *) chkp;
              soundFont->chunk.pmod= (sfModList *) &pmodChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w5", chkp + 8, pmodChunk->ckSize);
              chkp += pmodChunk->ckSize+8;
              j += pmodChunk->ckSize+8;
            }
            else if (chkid == s2d("pgen")) {
#ifdef BETA
              printf("pgen ");
#endif
              pgenChunk = (CHUNK *) chkp;
              soundFont->chunk.pgen= (sfGenList *) &pgenChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, pgenChunk->ckSize);
              chkp += pgenChunk->ckSize+8;
              j += pgenChunk->ckSize+8;
            }
            else if (chkid == s2d("inst")) {
#ifdef BETA
              printf("inst ");
#endif
              instChunk = (CHUNK *) chkp;
              soundFont->chunk.inst= (sfInst *) &instChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20w", chkp + 8, instChunk->ckSize);
              chkp += instChunk->ckSize+8;
              j += instChunk->ckSize+8;
            }
            else if (chkid == s2d("ibag")) {
#ifdef BETA
              printf("ibag ");
#endif
              ibagChunk = (CHUNK *) chkp;
              soundFont->chunk.ibag= (sfInstBag *) &ibagChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, ibagChunk->ckSize);
              chkp += ibagChunk->ckSize+8;
              j += ibagChunk->ckSize+8;
            }
            else if (chkid == s2d("imod")) {
#ifdef BETA
              printf("imod ");
#endif
              imodChunk = (CHUNK *) chkp;
              soundFont->chunk.imod= (sfInstModList *) &imodChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w5", chkp + 8, imodChunk->ckSize);
              chkp += imodChunk->ckSize+8;
              j += imodChunk->ckSize+8;
            }
            else if (chkid == s2d("igen")) {
#ifdef BETA
              printf("igen ");
#endif
              igenChunk = (CHUNK *) chkp;
              soundFont->chunk.igen= (sfInstGenList *) &igenChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, igenChunk->ckSize);
              chkp += igenChunk->ckSize+8;
              j += igenChunk->ckSize+8;
            }
            else if (chkid == s2d("shdr")) {
#ifdef BETA
              printf("shdr ");
#endif
              shdrChunk = (CHUNK *) chkp;
              soundFont->chunk.shdr= (sfSample *) &shdrChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20d5b2w2", chkp + 8, shdrChunk->ckSize);
              chkp += shdrChunk->ckSize+8;
              j += shdrChunk->ckSize+8;
            }
            else {
#ifdef BETA
              printf("Unknown sfont %.4s(%.8lx)\n", (char*)&chkid, chkid);
#endif
              shdrChunk = (CHUNK *) chkp;
              chkp += shdrChunk->ckSize+8;
              j += shdrChunk->ckSize+8;
            }
          } while (j < main_chunk->ckSize);
        }
        else {
#ifdef BETA
          printf("Unknown sfont %.4s(%.8lx)\n", (char*)&chkid, chkid);
#endif
          shdrChunk = (CHUNK *) chkp;
          chkp += shdrChunk->ckSize+8;
          j += shdrChunk->ckSize+8;
        }
      }
      else {
#ifdef BETA
        printf("Unknown sfont %.4s(%.8lx)\n", (char*)&chkid, chkid);
#endif
        shdrChunk = (CHUNK *) chkp;
        chkp += shdrChunk->ckSize+8;
        j += shdrChunk->ckSize+8;
      }
    }
    soundFont->chunk.smplChunk = smplChunk;
    soundFont->chunk.phdrChunk = phdrChunk;
    soundFont->chunk.pbagChunk = pbagChunk;
    soundFont->chunk.pmodChunk = pmodChunk;
    soundFont->chunk.pgenChunk = pgenChunk;
    soundFont->chunk.instChunk = instChunk;
    soundFont->chunk.ibagChunk = ibagChunk;
    soundFont->chunk.imodChunk = imodChunk;
    soundFont->chunk.igenChunk = igenChunk;
    soundFont->chunk.shdrChunk = shdrChunk;
}

#define S       sizeof

static OENTRY localops[] = {
{ "sfload",S(SFLOAD),     1,    "i",    "S",    (SUBR)SfLoad                  },
{ "sfpreset",S(SFPRESET), 1,    "i",    "iiii", (SUBR)SfPreset                },
{ "sfplay", S(SFPLAY), 5, "aa", "iixxioo",(SUBR)SfPlay_set, NULL, (SUBR)SfPlay },
{ "sfplaym", S(SFPLAYMONO), 5, "a", "iixxioo",(SUBR)SfPlayMono_set, NULL, (SUBR)SfPlayMono   },
{ "sfplist",S(SFPLIST),   1,    "",     "i",    (SUBR)Sfplist             },
{ "sfilist",S(SFPLIST),   1,    "",     "i",    (SUBR)Sfilist             },
{ "sfpassign",S(SFPASSIGN), 1,  "",     "ii",   (SUBR)SfAssignAllPresets  },
{ "sfinstrm", S(SFIPLAYMONO),5, "a", "iixxiioo", (SUBR)SfInstrPlayMono_set, NULL, (SUBR)SfInstrPlayMono   },
{ "sfinstr", S(SFIPLAY),  5,    "aa", "iixxiioo",(SUBR)SfInstrPlay_set, NULL,(SUBR)SfInstrPlay },
{ "sfplay3", S(SFPLAY),   5,    "aa", "iixxioo", (SUBR)SfPlay_set, NULL, (SUBR)SfPlay3  },
{ "sfplay3m", S(SFPLAYMONO), 5, "a", "iixxioo",  (SUBR)SfPlayMono_set, NULL,(SUBR)SfPlayMono3 },
{ "sfinstr3", S(SFIPLAY), 5,    "aa", "iixxiioo", (SUBR)SfInstrPlay_set, NULL, (SUBR)SfInstrPlay3 },
{ "sfinstr3m", S(SFIPLAYMONO), 5, "a", "iixxiioo", (SUBR)SfInstrPlayMono_set, NULL, (SUBR)SfInstrPlayMono3 },
};

LINKAGE
