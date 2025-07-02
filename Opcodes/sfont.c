/*
    sfont.c:

    Copyright (C) 2000-7 Gabriel Maldonado, John ffitch, Victor Lazzarini

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

/* WARNING! This file MUST be compiled by setting the structure member
   alignment (compiler option) to 1 byte. That is: no padding bytes
   should be present between a structure data member and another.
   This code will cause memory access faults and crash Csound if
   compiled with structure member alignment different than 1. See the
   documentation of your C compiler to choose the appropriate compiler
   directive switch.  */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#ifndef __wasi__
#include <errno.h>
#endif
#include "sfenum.h"
#include "sfont.h"

#define s2d(x)  *((DWORD *) (x))



static int32_t chunk_read(CSOUND *, FILE *f, CHUNK *chunk);
static void fill_SfPointers(CSOUND *);
static int32_t  fill_SfStruct(CSOUND *);
static void layerDefaults(layerType *layer);
static void splitDefaults(splitType *split);

#define MAX_SFONT               (10)
#define MAX_SFPRESET            (16384)
#define GLOBAL_ATTENUATION      (FL(0.3))

#define ONETWELTH               (0.08333333333333333333333333333)
#define TWOTOTWELTH             (1.05946309435929526456182529495)

typedef struct _sfontg {
  SFBANK *soundFont;
  SFBANK *sfArray;
  int32_t currSFndx;
  int32_t maxSFndx;
  presetType **presetp;
  SHORT **sampleBase;
  MYFLT pitches[128];
} sfontg;

static int32_t SoundFontLoad(CSOUND *csound, char *fname)
{
    FILE *fil;
    void *fd;
    int32_t i;
    SFBANK *soundFont;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));

    //soundFont = globals->soundFont;
    fd = csound->FileOpen(csound, &fil, CSFILE_STD, fname, "rb",
                             "SFDIR;SSDIR", CSFTYPE_SOUNDFONT, 0);
    if (UNLIKELY(fd == NULL)) {
      #ifndef __wasi__
      csound->ErrorMsg(csound,
                  Str("sfload: cannot open SoundFont file \"%s\" (error %s)"),
                  fname, strerror(errno));
      #else
      csound->ErrorMsg(csound, Str("sfload: cannot open SoundFont file \"%s\""), fname);
      #endif
      return -1;
    }
    for (i=0; i<globals->currSFndx+1; i++) {
      //printf("name[%d]: %s \n",  i, globals->sfArray[i].name);
      if (strcmp(fname, globals->sfArray[i].name)==0) {
        csound->Warning(csound, "%s already loaded", fname);
        return i;
      }
    }
    soundFont = &globals->sfArray[globals->currSFndx];
    /* if (UNLIKELY(soundFont==NULL)){ */
    /*   csound->ErrorMsg(csound, Str("Sfload: cannot use globals")); */
    /*   return; */
    /* } */
    strncpy(soundFont->name, csound->GetFileName(fd), 256);
    //soundFont->name[255]='\0';
    if (UNLIKELY(chunk_read(csound, fil, &soundFont->chunk.main_chunk)<0))
      csound->Message(csound, "%s", Str("sfont: failed to read file\n"));
    csound->FileClose(csound, fd);
    globals->soundFont = soundFont;
    fill_SfPointers(csound);
    fill_SfStruct(csound);
    return -1;
}

static int32_t compare(presetType * elem1, presetType *elem2)
{
    if (elem1->bank * 128 + elem1->prog >  elem2->bank * 128 + elem2->prog)
      return 1;
    else
      return -1;
}

/* syntax:
        ihandle SfLoad "filename"
*/

static char *Gfname;            /* NOT THREAD SAFE */

static int32_t SfLoad_(CSOUND *csound, SFLOAD *p, int32_t istring)
                                       /* open a file and return its handle */
{                                      /* the handle is simply a stack index */
    char *fname;
    int32_t hand;
    SFBANK *sf;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY(globals==NULL)) {
      return csound->InitError(csound, "%s", Str("sfload: could not open globals\n"));
    }
    if (istring) fname = csound->Strdup(csound, ((STRINGDAT *)p->fname)->data);
    else {
      if (IsStringCode(*p->fname))
        fname = csound->Strdup(csound, csound->GetString(csound,*p->fname));
      else fname = csound->StringArg2Name(csound,
                                NULL, p->fname, "sfont.",
                                0);
    }
    /*    strcpy(fname, (char*) p->fname); */
    Gfname = fname;
    hand = SoundFontLoad(csound, fname);
    if (hand<0) {
      *p->ihandle = (MYFLT) globals->currSFndx;
      sf = &globals->sfArray[globals->currSFndx];
      qsort(sf->preset, sf->presets_num, sizeof(presetType),
            (int32_t (*)(const void *, const void * )) compare);
      csound->Free(csound,fname);
      if (UNLIKELY(++globals->currSFndx>=globals->maxSFndx)) {
        globals->maxSFndx += 5;
        globals->sfArray = (SFBANK *)csound->ReAlloc(csound, globals->sfArray,
                  /* JPff fix */        globals->maxSFndx*sizeof(SFBANK));
        csound->Warning(csound, "%s", Str("Extending soundfonts"));
        if (globals->sfArray  == NULL) return NOTOK;
      }
      //printf("curr sf: %d \n", globals->currSFndx);
    }
    else *p->ihandle=hand;
    return OK;
}

static int32_t SfLoad(CSOUND *csound, SFLOAD *p){
  return SfLoad_(csound,p,0);
}

static int32_t SfLoad_S(CSOUND *csound, SFLOAD *p){
  return SfLoad_(csound,p,1);
}

static char *filter_string(char *s, char temp_string[24])
{
    int32_t i=0, j=0;
    int32_t c;
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

static int32_t Sfplist(CSOUND *csound, SFPLIST *p)
{
    sfontg *globals;
    SFBANK *sf;
    char temp_string[24];
    int32_t j;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY( *p->ihandle<0 || *p->ihandle>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    sf = &globals->sfArray[(int32_t) *p->ihandle];
    /* if (UNLIKELY(sf==NULL)) */
    /*   return csound->InitError(csound, "%s", Str("invalid soundfont")); */
    csound->Message(csound, Str("\nPreset list of \"%s\"\n"), sf->name);
    for (j =0; j < sf->presets_num; j++) {
      presetType *prs = &sf->preset[j];
      csound->Message(csound, Str("%3d) %-20s\tprog:%-3d bank:%d\n"),
                              j, filter_string(prs->name, temp_string),
                              prs->prog, prs->bank);
    }
    csound->Message(csound, "\n");
    return OK;
}

static int32_t SfAssignAllPresets(CSOUND *csound, SFPASSIGN *p)
{
    sfontg *globals;
    SFBANK *sf;
    int32_t pHandle, pnum;
    int32_t j, enableMsgs;

    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY( *p->ihandle<0 || *p->ihandle>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    sf = &globals->sfArray[(int32_t) *p->ihandle];
    /* if (UNLIKELY(globals->soundFont==NULL)) */
    /*   return csound->InitError(csound, "%s", Str("invalid sound font")); */

    pHandle = (int32_t) *p->startNum;
    pnum = sf->presets_num;
    enableMsgs = (*p->msgs==FL(0.0));
    if (enableMsgs)
      csound->Message(csound,
                      Str("\nAssigning all Presets of \"%s\" starting from"
                          " %d (preset handle number)\n"), sf->name, pHandle);
    for (j = 0; j < pnum; j++) {
      presetType *prs = &sf->preset[j];
      if (enableMsgs)
        csound->Message(csound, Str("%3d<--%-20s\t(prog:%-3d bank:%d)\n"),
                                j, prs->name, prs->prog, prs->bank);
      globals->presetp[pHandle] = &sf->preset[j];
      globals->sampleBase[pHandle] = sf->sampleData;
      pHandle++;
    }
    if (enableMsgs)
      csound->Message(csound, Str("\nAll presets have been assigned to preset"
                                  " handles from %d to %d\n\n"),
                              (int32_t) *p->startNum, pHandle - 1);

    return OK;
}

static int32_t Sfilist(CSOUND *csound, SFPLIST *p)
{
    sfontg *globals;
    SFBANK *sf;
    int32_t j;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY( *p->ihandle<0 || *p->ihandle>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    /* if (UNLIKELY(globals->soundFont==NULL)) */
    /*   return csound->InitError(csound, "%s", Str("invalid sound font")); */

    sf = &globals->sfArray[(int32_t) *p->ihandle];
    csound->Message(csound, Str("\nInstrument list of \"%s\"\n"), sf->name);
    for (j =0; j < sf->instrs_num; j++) {
      instrType *inst = &sf->instr[j];
      csound->Message(csound, "%3d) %-20s\n", j, inst->name);
    }
    csound->Message(csound, "\n");
    return OK;
}

static int32_t Sfilist_prefix(CSOUND *csound, SFPLIST *p)
{
    sfontg *globals;
    SFBANK *sf;
    int32_t j;
    char *prefix = p->Sprefix->data;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY( *p->ihandle<0 || *p->ihandle>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    /* if (UNLIKELY(globals->soundFont==NULL)) */
    /*   return csound->InitError(csound, "%s", Str("invalid sound font")); */

    sf = &globals->sfArray[(int32_t) *p->ihandle];
    csound->Message(csound, Str("\nInstrument list of \"%s\"\n"), sf->name);
    for (j =0; j < sf->instrs_num; j++) {
      instrType *inst = &sf->instr[j];
      csound->Message(csound, "%s%03d: %-20s\n", prefix, j, inst->name);
    }
    csound->Message(csound, "\n");
    return OK;
}


static int32_t SfPreset(CSOUND *csound, SFPRESET *p)
{
    sfontg *globals; SFBANK *sf;
    int32_t j, presetHandle = (int32_t) *p->iPresetHandle;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[(DWORD) *p->isfhandle];
    if (UNLIKELY( *p->isfhandle<0 || *p->isfhandle>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));

    if (UNLIKELY(presetHandle >= MAX_SFPRESET || presetHandle<0)) {
      return csound->InitError(csound,
                               Str("sfpreset: preset handle too big (%d), max: %d"),
                               presetHandle, (int32_t) MAX_SFPRESET - 1);
    }

    for (j=0; j< sf->presets_num; j++) {
      if (sf->preset[j].prog == (WORD) *p->iprog &&
          sf->preset[j].bank == (WORD) *p->ibank )
        {
          globals->presetp[presetHandle] = &sf->preset[j];
          globals->sampleBase[presetHandle] = sf->sampleData;
          break;
        }
    }
    *p->ipresethandle = (MYFLT) presetHandle;

    if (UNLIKELY(globals->presetp[presetHandle] == NULL)) {
      //      return csound->InitError(csound,
      csound->Warning(csound,
                               Str("sfpreset: cannot find any preset having prog "
                                   "number %d and bank number %d in SoundFont file"
                                   " \"%s\""),
                               (int32_t) *p->iprog, (int32_t) *p->ibank,
                               globals->sfArray[(DWORD) *p->isfhandle].name);
    }
    return OK;
}

static int32_t SfPlay_set(CSOUND *csound, SFPLAY *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;
    int32_t layersNum, j, spltNum = 0, flag = (int32_t) *p->iflag;
    sfontg *globals;

    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY(index>=MAX_SFPRESET))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];

    if (*p->iskip && p->spltNum) return OK;
    if (!UNLIKELY(preset!=NULL)) {
      return csound->InitError(csound, "%s", Str("sfplay: invalid or "
                                           "out-of-range preset number"));
    }
    layersNum = preset->layers_num;
    // csound->Message(csound, "sfplay: %d layers in preset %d-%s\n", layersNum, index, preset->name);
    for (j =0; j < layersNum; j++) {
      layerType *layer = &preset->layer[j];
      uint32_t vel = (uint32_t) abs((int32_t) *p->ivel),
        notnum = (uint32_t) abs((int32_t) *p->inotnum);
      /* csound->Message(csound, "layer: %d, vel:%d minvel: %d maxvel: %d" 
                                     "\n\t note: %d minnote: %d maxmote: %d \n",
                        j, vel, layer->minVelRange, layer->maxVelRange,
                        notnum, layer->minNoteRange, layer->maxNoteRange); */
      if (notnum >= layer->minNoteRange &&
          notnum <= layer->maxNoteRange &&
          vel    >= layer->minVelRange  &&
          vel    <= layer->maxVelRange) {
        int32_t splitsNum = layer->splits_num, k; 
        for (k = 0; k < splitsNum; k++) {
          splitType *split = &layer->split[k];
          /* csound->Message(csound, "split: %d, vel:%d minvel: %d maxvel: %d" 
                                     "\n\t note: %d minnote: %d maxmote: %d \n",
                         k, vel, split->minVelRange, split->maxVelRange,
                         notnum, split->minNoteRange, split->maxNoteRange); */
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
            int32_t orgkey = split->overridingRootKey, nm = notnum;
            if (orgkey == -1) orgkey = sample->byOriginalKey;
            orgfreq = globals->pitches[orgkey];
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*
                               sample->dwSampleRate*CS_ONEDSR;
            }
            else {
              freq = orgfreq*
                pow(2.0, ONETWELTH * tuneCorrection)*
                pow(2.0, ONETWELTH * (split->scaleTuning*0.01) * (nm-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*CS_ONEDSR;
            }
              ;
            attenuation = (MYFLT) (layer->initialAttenuation +
                                   split->initialAttenuation);
            attenuation = POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * attenuation )
              * GLOBAL_ATTENUATION;
            pan = (double)(split->pan + layer->pan) / 1000.0 + 0.5;
            if (pan > 1.0) pan = 1.0;
            else if (pan < 0.0) pan = 0.0;
            /* Suggested fix from steven yi Oct 2002 */
            p->base[spltNum] = sBase + start;
            p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
            p->end[spltNum] = (DWORD) (sample->dwEnd + split->endOffset - start);
            p->startloop[spltNum] =  (DWORD) 
              (sample->dwStartloop + split->startLoopOffset  - start);
            p->endloop[spltNum] =  (DWORD) 
              (sample->dwEndloop + split->endLoopOffset - start);
            p->leftlevel[spltNum] = (MYFLT) sqrt(1.0-pan) * attenuation;
            p->rightlevel[spltNum] = (MYFLT) sqrt(pan) * attenuation;
            p->mode[spltNum]= split->sampleModes;
            p->attack[spltNum] = split->attack*CS_EKR;
            p->decay[spltNum] = split->decay*CS_EKR;
            p->sustain[spltNum] = split->sustain;
            p->release[spltNum] = split->release*CS_EKR;

            if (*p->ienv > 1) {
              p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
              p->decr[spltNum] = pow((split->sustain+0.0001),
                                     1.0/(CS_EKR*
                                          split->decay+0.0001));
              if (split->attack != 0.0) p->env[spltNum] = 0.0;
              else p->env[spltNum] = 1.0;
            }
            else if (*p->ienv > 0) {
              p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
              p->decr[spltNum] = (split->sustain-1.0)/(CS_EKR*
                                                       split->decay);
              if (split->attack != 0.0) p->env[spltNum] = 0.0;
              else p->env[spltNum] = 1.0;
            }
            else {
              p->env[spltNum] = 1.0;
            }
            p->ti[spltNum] = 0;
            /*csound->Message(csound, "play: split %d, samplebase:%p freq: %f orig: %f" 
                                     "\n\t atten:%f pan:%f mode:%d \n",
                            k, p->base[spltNum],
                            freq, orgfreq,  attenuation, pan, split->sampleModes);*/
            spltNum++;
          }
        }
      }
    }
    p->spltNum = spltNum;
    
    return OK;
}

#define Linear_interpolation \
        SHORT *curr_samp = *base + (int32) *phs;\
        MYFLT fract = (MYFLT) *phs - (MYFLT)((int32)*phs);\
        MYFLT out = (*curr_samp + (*(curr_samp+1) - *curr_samp)*fract);

#define Cubic_interpolation \
        MYFLT phs1 = (MYFLT) *phs -FL(1.0);\
        int32_t   x0 = (int32)phs1 ;\
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

#define ExpEnvelope \
        if (*tinc < *attack) *env += *attr;                        \
        else if (*tinc < *decay + *attack) *env *= *decr;          \
        else *env = *sustain;                                      \
        (*tinc)++;                                                 \

#define LinEnvelope \
        if (*tinc < *attack) *env += *attr;                        \
        else if (*tinc < *decay + *attack) *env += *decr;          \
        else *env = *sustain;                                      \
        (*tinc)++;                                                 \

#define Unlooped \
        if (*phs > *end) break;           \
        if (*phs < FL(0.0)) *phs = FL(0.0);       \

#define Mono_out \
        out1[n] +=  *attenuation * out * (*env); \
        *phs += si;

#define Stereo_out \
        out1[n] += *left * out * (*env);\
        out2[n] += *right * out * (*env);\
        *phs += si;

static int32_t SfPlay(CSOUND *csound, SFPLAY *p)
{
    IGN(csound);
    MYFLT   *out1 = p->out1, *out2 = p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop= p->startloop, *endloop= p->endloop,
          *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
      *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
      /* *release = p->release,*/ *attr = p->attr;


    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation Stereo_out Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation Stereo_out  Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation Stereo_out     Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation Stereo_out Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
        out2[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
        out2[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfPlay3(CSOUND *csound, SFPLAY *p)
{
    IGN(csound);
    MYFLT    *out1 = p->out1, *out2 = p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop = p->startloop,
          *endloop = p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
          *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
          /**release = p->release,*/ *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;
/*         nsmps = CS_KSMPS; */
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Stereo_out      Looped
              
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Stereo_out      Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
         
        double looplength = *endloop - *startloop, si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
           
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation
              Stereo_out
              Looped
              
          }
          
        }
        else if (*phs < *end) {
         
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Stereo_out      Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
        
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
        out2[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
        out2[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfPlayMono_set(CSOUND *csound, SFPLAYMONO *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;
    int32_t layersNum, j, spltNum = 0, flag=(int32_t) *p->iflag;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    //printf("*** index= %d  maximum = %d\n", index, globals->currSFndx);
    if (UNLIKELY(index>=MAX_SFPRESET))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));

    if (*p->iskip && p->spltNum) return OK;
    
    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];

    if (UNLIKELY(!preset)) {
      return csound->InitError(csound, "%s", Str("sfplaym: invalid or "
                                           "out-of-range preset number"));
    }
    layersNum= preset->layers_num;
    for (j =0; j < layersNum; j++) {
      layerType *layer = &preset->layer[j];
      uint32_t vel= (uint32_t) abs((int32_t) *p->ivel),
        notnum= (uint32_t) abs((int32_t)*p->inotnum);
      if (notnum >= layer->minNoteRange &&
          notnum <= layer->maxNoteRange &&
          vel >= layer->minVelRange  &&
          vel <= layer->maxVelRange) {
        int32_t splitsNum = layer->splits_num, k;
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
            int32_t orgkey = split->overridingRootKey, nn = notnum;
            if (orgkey == -1) orgkey = sample->byOriginalKey;
            orgfreq = globals->pitches[orgkey] ;
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*
                               sample->dwSampleRate*CS_ONEDSR;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (nn-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*CS_ONEDSR;
            }
            p->attenuation[spltNum] =
              POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * (layer->initialAttenuation +
                                                    split->initialAttenuation)) *
              GLOBAL_ATTENUATION;
            p->base[spltNum] =  sBase+ start;
            p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
            p->end[spltNum] =  (DWORD) (sample->dwEnd + split->endOffset - start);
            p->startloop[spltNum] =  (DWORD) (sample->dwStartloop +
                                              split->startLoopOffset - start);
            p->endloop[spltNum] =  (DWORD)
              (sample->dwEndloop + split->endLoopOffset - start);
            p->mode[spltNum]= split->sampleModes;
            p->attack[spltNum] = split->attack*CS_EKR;
            p->decay[spltNum] = split->decay*CS_EKR;
            p->sustain[spltNum] = split->sustain;
            p->release[spltNum] = split->release*CS_EKR;

            if (*p->ienv > 1) {
             p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
             p->decr[spltNum] = pow((split->sustain+0.0001),
                                    1.0/(CS_EKR*
                                         split->decay+0.0001));
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
            }
            else if (*p->ienv > 0) {
            p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
            p->decr[spltNum] = (split->sustain-1.0)/(CS_EKR*
                                                     split->decay);
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
            }
            else {
              p->env[spltNum] = 1.0;
            }
            p->ti[spltNum] = 0;
            spltNum++;
          }
        }
      }
    }
    p->spltNum = spltNum;
    return OK;
}

static int32_t SfPlayMono(CSOUND *csound, SFPLAYMONO *p)
{
    IGN(csound);
    MYFLT   *out1 = p->out1 , *env  = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end, *startloop= p->startloop, *endloop= p->endloop,
          *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation, *attack = p->attack, *decr = p->decr,
          *decay = p->decay, *sustain= p->sustain, /**release = p->release,*/
          *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            if (*p->ienv > 1) { ExpEnvelope }
            else if (*p->ienv > 0) { LinEnvelope }
            { Linear_interpolation Mono_out Looped }
          }
        }
        else if (*phs < *end) {
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            if (*p->ienv > 1) { ExpEnvelope }
            else if (*p->ienv > 0) { LinEnvelope }
            { Linear_interpolation Mono_out Unlooped }
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          for (n=offset;n<nsmps;n++) {
            if (*p->ienv > 1) { ExpEnvelope }
            else if (*p->ienv > 0) { LinEnvelope }
            { Linear_interpolation Mono_out Looped }
          }
        }
        else if (*phs < *end) {
          for (n=offset;n<nsmps;n++) {
            if (*p->ienv > 1) { ExpEnvelope }
            else if (*p->ienv > 0) { LinEnvelope }
            { Linear_interpolation Mono_out Unlooped }
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        /*release++;*/ tinc++; env++; attr++; decr++;
      }
    }
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfPlayMono3(CSOUND *csound, SFPLAYMONO *p)
{
    IGN(csound);
    MYFLT   *out1 = p->out1, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT  **base = p->base;
    DWORD   *end = p->end,  *startloop = p->startloop,
            *endloop = p->endloop, *tinc = p->ti;
    SHORT   *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation,*attack = p->attack, *decr = p->decr,
          *decay = p->decay, *sustain= p->sustain, /**release = p->release,*/
          *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;
    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Mono_out        Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Mono_out        Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Mono_out Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Mono_out Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
      }
    }

    return OK;
}

static int32_t SfInstrPlay_set(CSOUND *csound, SFIPLAY *p)
{
    sfontg *globals;
    SFBANK *sf;
    int32_t index = (int32_t) *p->sfBank;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY(index>=MAX_SFPRESET))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));
    sf = &globals->sfArray[index];
    if (*p->iskip && p->spltNum)  return OK;
    if (UNLIKELY(*p->instrNum >  sf->instrs_num)) {
      return csound->InitError(csound, "%s", Str("sfinstr: instrument out of range"));
    }
    else {
      instrType *layer = &sf->instr[(int32_t) *p->instrNum];
      SHORT *sBase = sf->sampleData;
      int32_t spltNum = 0, flag=(int32_t) *p->iflag;
      uint32_t vel= (uint32_t) abs((int32_t) *p->ivel),
        notnum= (uint32_t) abs((int32_t)*p->inotnum);
      uint32_t splitsNum = layer->splits_num, k;
      
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
          int32_t orgkey = split->overridingRootKey, nn = notnum;
          if (orgkey == -1) orgkey = sample->byOriginalKey;
          orgfreq = globals->pitches[orgkey] ;
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*
                              sample->dwSampleRate*CS_ONEDSR;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01)*(nn-orgkey));
            p->si[spltNum] = (freq/orgfreq)*(sample->dwSampleRate*CS_ONEDSR);
          }
          attenuation = (MYFLT) (split->initialAttenuation);
          attenuation = POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * attenuation) *
            GLOBAL_ATTENUATION;
          pan = (MYFLT)  split->pan / FL(1000.0) + FL(0.5);
          if (pan > FL(1.0)) pan =FL(1.0);
          else if (pan < FL(0.0)) pan = FL(0.0);
          p->base[spltNum] = sBase + start;
          p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
          p->end[spltNum] =  (DWORD) (sample->dwEnd + split->endOffset - start);
          p->startloop[spltNum] =  (DWORD) (sample->dwStartloop +
                                            split->startLoopOffset - start);
          p->endloop[spltNum] =  (DWORD) (sample->dwEndloop +
                                          split->endLoopOffset - start);
          p->leftlevel[spltNum] = (FL(1.0)-pan) * attenuation;
          p->rightlevel[spltNum] = pan * attenuation;
          p->mode[spltNum]= split->sampleModes;

          p->attack[spltNum] = split->attack*CS_EKR;
          p->decay[spltNum] = split->decay*CS_EKR;
          p->sustain[spltNum] = split->sustain;
          p->release[spltNum] = split->release*CS_EKR;

          if (*p->ienv > 1) {
            p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
            p->decr[spltNum] = pow((split->sustain+0.0001),
                                   1.0/(CS_EKR*split->decay+0.0001));
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
          }
          else if (*p->ienv > 0) {
            p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
            p->decr[spltNum] = (split->sustain-1.0)/(CS_EKR*
                                                     split->decay);
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
          }
          else {
            p->env[spltNum] = 1.0;
          }
          p->ti[spltNum] = 0;
          spltNum++;
        }
      }
      p->spltNum = spltNum;
      
    }
    return OK;
}

static int32_t SfInstrPlay(CSOUND *csound, SFIPLAY *p)
{
    IGN(csound);
    MYFLT *out1= p->out1, *out2= p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
          *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
          /**release = p->release,*/ *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation        Stereo_out      Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation Stereo_out     Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        /*release++;*/ tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation        Stereo_out      Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation        Stereo_out      Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }

    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
        out2[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
        out2[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfInstrPlay3(CSOUND *csound, SFIPLAY *p)
{
   IGN(csound);
    MYFLT *out1= p->out1, *out2= p->out2,*env =p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel,
      *attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, /**release = p->release,*/
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Stereo_out      Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Stereo_out      Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        /*release++;*/ tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Stereo_out      Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Stereo_out      Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        left++; right++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }

    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
        out2[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
        out2[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfInstrPlayMono_set(CSOUND *csound, SFIPLAYMONO *p)
{
    int32_t index = (int32_t) *p->sfBank;
    sfontg *globals;
    SFBANK *sf;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY(index<0 || index>=globals->currSFndx))
      return csound->InitError(csound, "%s", Str("invalid soundfont"));

    sf = &globals->sfArray[index];
    if (UNLIKELY( *p->instrNum >  sf->instrs_num)) {
      return csound->InitError(csound, "%s", Str("sfinstr: instrument out of range"));
    }
    else {
      instrType *layer = &sf->instr[(int32_t) *p->instrNum];
      SHORT *sBase = sf->sampleData;
      int32_t spltNum = 0, flag=(int32_t) *p->iflag;
      uint32_t vel= (int32_t) *p->ivel, notnum= (int32_t) *p->inotnum;
      int32_t splitsNum = layer->splits_num, k;

      if (*p->iskip && p->spltNum) return OK;
     
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
          int32_t orgkey = split->overridingRootKey, nn = (int32_t) notnum;
          if (orgkey == -1) orgkey = sample->byOriginalKey;
          orgfreq = globals->pitches[orgkey];
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*
                              sample->dwSampleRate*CS_ONEDSR;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (nn-orgkey));
            p->si[spltNum] = (freq/orgfreq)*(sample->dwSampleRate*CS_ONEDSR);
          }
          p->attenuation[spltNum] = (MYFLT) pow(2.0, (-1.0/60.0)*
                                                split->initialAttenuation)
            * GLOBAL_ATTENUATION;
          p->base[spltNum] = sBase+ start;
          p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
          p->end[spltNum] =  (DWORD) (sample->dwEnd + split->endOffset - start);
          p->startloop[spltNum] =  (DWORD) (sample->dwStartloop +
                                            split->startLoopOffset - start);
          p->endloop[spltNum] =  (DWORD) (sample->dwEndloop +
                                          split->endLoopOffset - start);
          p->mode[spltNum]= split->sampleModes;
          p->attack[spltNum] = split->attack*CS_EKR;
          p->decay[spltNum] = split->decay*CS_EKR;
          p->sustain[spltNum] = split->sustain;
          p->release[spltNum] = split->release*CS_EKR;

          if (*p->ienv > 1) {
            p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
            p->decr[spltNum] = pow((split->sustain+0.0001),
                                   1.0/(CS_EKR*
                                        split->decay+0.0001));
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
          }
          else if (*p->ienv > 0) {
            p->attr[spltNum] = 1.0/(CS_EKR*split->attack);
            p->decr[spltNum] = (split->sustain-1.0)/(CS_EKR*
                                                     split->decay);
            if (split->attack != 0.0) p->env[spltNum] = 0.0;
            else p->env[spltNum] = 1.0;
          }
          else {
            p->env[spltNum] = 1.0;
          }
          p->ti[spltNum] = 0;
          spltNum++;
        }
      }
      p->spltNum = spltNum;
    }
    return OK;
}

static int32_t SfInstrPlayMono(CSOUND *csound, SFIPLAYMONO *p)
{
    IGN(csound);
    MYFLT *out1= p->out1, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
     uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop,
      *tinc = p->ti;
    SHORT *mode = p->mode;

    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation, *attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, /**release = p->release,*/
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation        Mono_out        Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Linear_interpolation Mono_out       Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        /*release++;*/ tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation Mono_out Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Linear_interpolation Mono_out Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        /*release++;*/ tinc++; env++; attr++; decr++;
      }
    }
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
      }
    }
    return OK;
}

static int32_t SfInstrPlayMono3(CSOUND *csound, SFIPLAYMONO *p)
{
    IGN(csound);
    MYFLT *out1= p->out1, *env = p->env  ;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation,*attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, /**release = p->release,*/
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Mono_out Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            double si = *sampinc * freq[n];
            Cubic_interpolation Mono_out Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int32_t flag =0;
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Mono_out Looped
          }
        }
        else if (*phs < *end) {
          if (*p->ienv > 1) { ExpEnvelope }
          else if (*p->ienv > 0) { LinEnvelope }
          for (n=offset;n<nsmps;n++) {
            Cubic_interpolation Mono_out Unlooped
          }
        }
        phs++; base++; sampinc++; endloop++; startloop++;
        attenuation++, mode++, end++; attack++; decay++; sustain++;
        tinc++; env++; attr++; decr++;
      }
    }
    if (IS_ASIG_ARG(p->xamp)) {
      MYFLT *amp = p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= amp[n];
      }
    }
    else {
      MYFLT famp = *p->xamp;
      for (n=offset;n<nsmps;n++) {
        out1[n] *= famp;
      }
    }
    return OK;
}

/*********************/

/*  Convert Big-endian <-> Little-endian (for Big-endian machines only)
 *  fmt: ((b|w|d)[0-9]*)+
 *  b:byte (no conversion), w:word, d:double word, digits(optional):repeat n times
 */
#ifdef WORDS_BIGENDIAN
static void ChangeByteOrder(char *fmt, char *p, int32 size)
{
    char c, c1, c2, c3, c4;
    char *fmt_org = fmt;
    int32 i, times;

    while (size > 0) {
      fmt = fmt_org;
      while (*fmt) {
        c = *fmt++;
        if (isdigit(*fmt)) {
          times = strtol(fmt, &fmt, 10);
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

static int32_t fill_SfStruct(CSOUND *csound)
{
    int32_t j, k, i, l, m, size, iStart, iEnd, kk, ll, mStart, mEnd;
    int32_t pbag_num,first_pbag,layer_num;
    int32_t ibag_num,first_ibag,split_num;
    CHUNK *phdrChunk;
    presetType *preset;
    sfPresetHeader *phdr;
    sfPresetBag *pbag;
    sfGenList *pgen;
    sfInst *inst;
    sfInstBag *ibag;
/*  sfInstModList *imod; */
    sfInstGenList *igen;
    sfSample *shdr;
    SFBANK *soundFont;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    soundFont = globals->soundFont;


/*  imod = soundFont->chunk.imod; */
    igen = soundFont->chunk.igen;
    shdr = soundFont->chunk.shdr;

    phdrChunk= soundFont->chunk.phdrChunk;
    phdr = soundFont->chunk.phdr;
    pbag = soundFont->chunk.pbag;
    pgen = soundFont->chunk.pgen;
    inst = soundFont->chunk.inst;
    ibag = soundFont->chunk.ibag;

    size = phdrChunk->ckSize / sizeof(sfPresetHeader);
    soundFont->presets_num = size;
    preset = (presetType *) csound->Malloc(csound, size * sizeof(presetType));
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
      preset[j].layers_num = layer_num;
      preset[j].layer =
        (layerType *) csound->Malloc(csound, layer_num * sizeof(layerType));
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
              int32_t GsampleModes=UNUSE, GcoarseTune=UNUSE, GfineTune=UNUSE;
              int32_t Gpan=UNUSE, GinitialAttenuation=UNUSE,GscaleTuning=UNUSE;
              int32_t GoverridingRootKey = UNUSE;

              layer->num  = pgen[i].genAmount.wAmount;
              layer->name = inst[layer->num].achInstName;
              first_ibag = inst[layer->num].wInstBagNdx;
              ibag_num = inst[layer->num +1].wInstBagNdx - first_ibag;
              split_num = 0;
              for (l=0; l < ibag_num; l++) {
                mStart = ibag[l+first_ibag].wInstGenNdx;
                mEnd = ibag[l+first_ibag+1].wInstGenNdx;
                for (m=mStart; m < mEnd; m++) {
                  if (igen[m].sfGenOper == sampleID) {
                    split_num++;
                  }
                }
              }
              layer->splits_num = split_num;
              layer->split =
                (splitType *) csound->Malloc(csound, split_num * sizeof(splitType));
              for (l=0; l<split_num; l++) {
                splitDefaults(&layer->split[l]);
              }
              for (l=0, ll=0; l < ibag_num; l++) {
                int32_t sglobal_zone = 1;
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
                      GoverridingRootKey = igen[m].genAmount.shAmount;
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
                  split->attack = split->decay = split->sustain =
                    split->release = FL(0.0);
                  if (GoverridingRootKey != UNUSE)
                    split->overridingRootKey = (SBYTE) GoverridingRootKey;
                  if (GcoarseTune != UNUSE)
                    split->coarseTune = (SBYTE) GcoarseTune;
                  if (GfineTune != UNUSE)
                    split->fineTune = (SBYTE) GfineTune;
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
                        int32_t num = igen[m].genAmount.wAmount;
                        split->num= num;
                        split->sample = &shdr[num];
                        if (UNLIKELY(split->sample->sfSampleType & 0x8000)) {
                          csound->Free(csound, preset);
                          csound->ErrorMsg(csound, Str("SoundFont file \"%s\" "
                                                       "contains ROM samples !\n"
                                                       "At present time only RAM "
                                                       "samples are allowed "
                                                       "by sfload.\n"
                                                       "Session aborted !"),
                                           Gfname);
                            return NOTOK;
                        }
                        //sglobal_zone = 0;
                        ll++;
                      }
                      break;
                    case overridingRootKey:
                      split->overridingRootKey = (SBYTE) igen[m].genAmount.shAmount;
                      break;
                    case coarseTune:
                      split->coarseTune = (/*char*/ SBYTE) igen[m].genAmount.shAmount;
                      break;
                    case fineTune:
                      split->fineTune = (/*char*/ SBYTE) igen[m].genAmount.shAmount;
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
                    case delayVolEnv:
                      // csound->Message(csound, "del: %f\n",
                      //                 (double) igen[m].genAmount.shAmount);
                      break;
                    case attackVolEnv:           /*attack */
                      split->attack = POWER(FL(2.0),
                                            igen[m].genAmount.shAmount/FL(1200.0));
                      /* csound->Message(csound, "att: %f\n", split->attack ); */
                      break;
                      /* case holdVolEnv: */             /*hold   35 */
                    case decayVolEnv:            /*decay */
                      split->decay = POWER(FL(2.0),
                                           igen[m].genAmount.shAmount/FL(1200.0));
                      /* csound->Message(csound, "dec: %f\n", split->decay); */
                      break;
                    case sustainVolEnv:          /*sustain */
                      split->sustain = POWER(FL(10.0),
                                             -igen[m].genAmount.shAmount/FL(20.0));
                      /* csound->Message(csound, "sus: %f\n", split->sustain); */
                      break;
                    case releaseVolEnv:          /*release */
                      split->release = POWER(FL(2.0),
                                             igen[m].genAmount.shAmount/FL(1200.0));
                      /* csound->Message(csound, "rel: %f\n", split->release); */
                      break;
                    case keynum:
                      /*csound->Message(csound, "");*/
                      break;
                    case velocity:
                      /*csound->Message(csound, "");*/
                      break;
                    case exclusiveClass:
                      /*csound->Message(csound, "");*/
                      break;

                    }
                  }
                }
              }
              kk++;
            }
            break;
          case coarseTune:
            layer->coarseTune = (/*char*/ SBYTE) pgen[i].genAmount.shAmount;
            break;
          case fineTune:
            layer->fineTune = (/*char*/ SBYTE) pgen[i].genAmount.shAmount;
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
      instru = (instrType *) csound->Malloc(csound, size * sizeof(layerType));
      for (j=0; j < size; j++) {
#define UNUSE 0x7fffffff
        int32_t GsampleModes=UNUSE, GcoarseTune=UNUSE, GfineTune=UNUSE;
        int32_t Gpan=UNUSE, GinitialAttenuation=UNUSE,GscaleTuning=UNUSE;
        int32_t GoverridingRootKey = UNUSE;

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
        instru[j].split =
          (splitType *) csound->Malloc(csound, split_num * sizeof(splitType));
        for (l=0; l<split_num; l++) {
          splitDefaults(&instru[j].split[l]);
        }
        for (l=0, ll=0; l < ibag_num; l++) {
          int32_t sglobal_zone = 1;
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
                GoverridingRootKey = igen[m].genAmount.shAmount;
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
              split->overridingRootKey = (SBYTE) GoverridingRootKey;
            if (GcoarseTune != UNUSE)
              split->coarseTune = (SBYTE) GcoarseTune;
            if (GfineTune != UNUSE)
              split->fineTune = (SBYTE) GfineTune;
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
                  int32_t num = igen[m].genAmount.wAmount;
                  split->num= num;
                  split->sample = &shdr[num];
                  if (UNLIKELY(split->sample->sfSampleType & 0x8000)) {
                    csound->Free(csound, instru);
                    csound->ErrorMsg(csound, Str("SoundFont file \"%s\" contains "
                                            "ROM samples !\n"
                                            "At present time only RAM samples "
                                            "are allowed by sfload.\n"
                                            "Session aborted !"), Gfname);
                    return NOTOK;
                  }
                  //sglobal_zone = 0;
                  ll++;
                }
                break;
              case overridingRootKey:
                split->overridingRootKey = (/*char*/ SBYTE) igen[m].genAmount.shAmount;
                break;
              case coarseTune:
                split->coarseTune = (/*char*/ SBYTE) igen[m].genAmount.shAmount;
                break;
              case fineTune:
                split->fineTune = (/*char*/ SBYTE) igen[m].genAmount.shAmount;
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
                /*csound->Message(csound, "");*/
                break;
              case velocity:
                /*csound->Message(csound, "");*/
                break;
              case exclusiveClass:
                /*csound->Message(csound, "");*/
                break;
              }
            }
          }
        }
      }
    end_fill_layers:
      soundFont->instr = instru;
    }
    return OK;
}

static void layerDefaults(layerType *layer)
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

static void splitDefaults(splitType *split)
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

static int32_t chunk_read(CSOUND *csound, FILE *fil, CHUNK *chunk)
{
    if (UNLIKELY(4 != fread(chunk->ckID,1,4, fil)))
      return 0;
    if (UNLIKELY(1 != fread(&chunk->ckSize,4,1,fil))) {
      chunk->ckSize = 0;
      return 0;
    }
    //if (UNLIKELY(chunk->ckSize>0x8fffff00)) return 0;
    ChangeByteOrder("d", (char *)&chunk->ckSize, 4);
    chunk->ckDATA = (BYTE *) csound->Malloc(csound, chunk->ckSize);
    if (chunk->ckDATA==NULL)
      return 0;
    if (chunk->ckSize>0x8fffff00) return 0;
    return (int32_t) fread(chunk->ckDATA,1,chunk->ckSize,fil);
}

static DWORD dword(char *p)
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

static void fill_SfPointers(CSOUND *csound)
{
    char *chkp;
    DWORD chkid, j, size;

    CHUNK *main_chunk;
    CHUNK *smplChunk=NULL, *phdrChunk=NULL, *pbagChunk=NULL, *pmodChunk=NULL;
    CHUNK *pgenChunk=NULL, *instChunk=NULL, *ibagChunk=NULL, *imodChunk=NULL;
    CHUNK *igenChunk=NULL, *shdrChunk=NULL;

    SFBANK *soundFont;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));

    if (UNLIKELY(globals == NULL)) {
      csound->ErrorMsg(csound, "%s", Str("Sfont: cannot use globals/"));
      return;
    }

    soundFont = globals->soundFont;
    if (LIKELY(soundFont != NULL))
      main_chunk=&(soundFont->chunk.main_chunk);
    else  {
     csound->ErrorMsg(csound, "%s", Str("Sfont: cannot use globals/"));
     return;
    }

    if (UNLIKELY(main_chunk->ckDATA == NULL)) {
      csound->ErrorMsg(csound, "%s", Str("Sfont format not compatible"));
      return;
    }
    chkp = (char *) main_chunk->ckDATA+4;

    for  (j=4; j< main_chunk->ckSize;) {

      chkid = /* (DWORD *) chkp*/ dword(chkp);
/* #ifdef BETA */
/*    csound->Message(csound, "Looking at %.4s\n", (char*) &chkid); */
/* #endif */
      if (chkid == s2d("LIST")) {
/* #ifdef BETA */
/*         csound->Message(csound, "LIST "); */
/* #endif */
        j += 4; chkp += 4;
        ChangeByteOrder("d", chkp, 4);
        size = /* (DWORD *) chkp */ dword(chkp);
        j += 4; chkp += 4;
        chkid = /* (DWORD *) chkp */ dword(chkp);
/* #ifdef BETA */
/*         csound->Message(csound, "**chkid %p %p\n", */
/*                                 (void*) chkid, (void*) (*((DWORD *) chkp))); */
/*         csound->Message(csound, ":Looking at %.4s (%u)\n", */
/*                                 (char*) &chkid, (uint32_t) size); */
/* #endif */
        if (chkid == s2d("INFO")) {
          chkp += size;
          j    += size;
        }
        else if (chkid == s2d("sdta")) {
          j +=4; chkp += 4;
          smplChunk = (CHUNK *) chkp;
          soundFont->sampleData = (void *) &(smplChunk->ckDATA);
          ChangeByteOrder("d", chkp + 4, 4);
          ChangeByteOrder("w", chkp + 8, size - 12);
/* #ifdef BETA */
/*           { */
/*             DWORD i; */
/*             for (i=size-12; i< size+4; i++) */
/*               csound->Message(csound, "%c(%.2x)", chkp[i], chkp[i]); */
/*             csound->Message(csound, "\n"); */
/*           } */
/* #endif */
          chkp += size-4;
          j += size-4;
        }
        else if (chkid  ==  s2d("pdta")) {
          j += 4; chkp += 4;
          do {
            chkid = /* (DWORD *) chkp */ dword(chkp);
            /* csound->Message(csound, "::Looking at %.4s (%d)\n",&chkid,size); */
            if (chkid == s2d("phdr")) {
              phdrChunk = (CHUNK *) chkp;
              soundFont->chunk.phdr= (sfPresetHeader *) &phdrChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20w3d3", chkp + 8, phdrChunk->ckSize);
              chkp += phdrChunk->ckSize+8;
              j += phdrChunk->ckSize+8;
            }
            else if (chkid == s2d("pbag")) {
              pbagChunk = (CHUNK *) chkp;
              soundFont->chunk.pbag= (void *) &pbagChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, pbagChunk->ckSize);
              chkp += pbagChunk->ckSize+8;
              j += pbagChunk->ckSize+8;
            }
            else if (chkid == s2d("pmod")) {
              pmodChunk = (CHUNK *) chkp;
              soundFont->chunk.pmod= (void *) &pmodChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w5", chkp + 8, pmodChunk->ckSize);
              chkp += pmodChunk->ckSize+8;
              j += pmodChunk->ckSize+8;
            }
            else if (chkid == s2d("pgen")) {
              pgenChunk = (CHUNK *) chkp;
              soundFont->chunk.pgen= (void *) &pgenChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, pgenChunk->ckSize);
              chkp += pgenChunk->ckSize+8;
              j += pgenChunk->ckSize+8;
            }
            else if (chkid == s2d("inst")) {
              instChunk = (CHUNK *) chkp;
              soundFont->chunk.inst= (sfInst *) &instChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20w", chkp + 8, instChunk->ckSize);
              chkp += instChunk->ckSize+8;
              j += instChunk->ckSize+8;
            }
            else if (chkid == s2d("ibag")) {
              ibagChunk = (CHUNK *) chkp;
              soundFont->chunk.ibag= (void *) &ibagChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, ibagChunk->ckSize);
              chkp += ibagChunk->ckSize+8;
              j += ibagChunk->ckSize+8;
            }
            else if (chkid == s2d("imod")) {
              imodChunk = (CHUNK *) chkp;
              soundFont->chunk.imod= (void *) &imodChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w5", chkp + 8, imodChunk->ckSize);
              chkp += imodChunk->ckSize+8;
              j += imodChunk->ckSize+8;
            }
            else if (chkid == s2d("igen")) {
              igenChunk = (CHUNK *) chkp;
              soundFont->chunk.igen= (sfInstGenList *) &igenChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("w2", chkp + 8, igenChunk->ckSize);
              chkp += igenChunk->ckSize+8;
              j += igenChunk->ckSize+8;
            }
            else if (chkid == s2d("shdr")) {
              shdrChunk = (CHUNK *) chkp;
              soundFont->chunk.shdr= (sfSample *) &shdrChunk->ckDATA;
              ChangeByteOrder("d", chkp + 4, 4);
              ChangeByteOrder("b20d5b2w2", chkp + 8, shdrChunk->ckSize);
              chkp += shdrChunk->ckSize+8;
              j += shdrChunk->ckSize+8;
            }
            else {
/* #ifdef BETA */
/*               csound->Message(csound, "Unknown sfont %.4s(%.8x)\n", */
/*                                       (char*) &chkid, (uint32_t) chkid); */
/* #endif */
              shdrChunk = (CHUNK *) chkp;
              chkp += shdrChunk->ckSize+8;
              j += shdrChunk->ckSize+8;
            }
          } while (j < main_chunk->ckSize);
        }
        else {
/* #ifdef BETA */
/*           csound->Message(csound, "Unknown sfont %.4s(%.8x)\n", */
/*                                   (char*) &chkid, (uint32_t) chkid); */
/* #endif */
          shdrChunk = (CHUNK *) chkp;
          chkp += shdrChunk->ckSize+8;
          j += shdrChunk->ckSize+8;
        }
      }
      else {
/* #ifdef BETA */
/*         csound->Message(csound, "Unknown sfont %.4s(%.8x)\n", */
/*                                 (char*) &chkid, (uint32_t) chkid); */
/* #endif */
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

typedef struct _sflooper {
  OPDS h;
  MYFLT *outL, *outR;  /* output */
  MYFLT *ivel, *inotnum, *amp, *pitch, *ipresethandle, *loop_start, *loop_end,
    *crossfade, *start, *imode, *ifn2, *iskip, *iflag;
  int32_t     spltNum;
  SHORT   *sBase[MAXSPLT];
  FUNC *efunc;
  MYFLT count;
  int32_t lstart[MAXSPLT], lend[MAXSPLT], cfade, mode;
  double  ndx[MAXSPLT][2];    /* table lookup ndx */
  double  freq[MAXSPLT];
  int32_t firsttime[MAXSPLT], init, end[MAXSPLT], sstart[MAXSPLT];
  MYFLT   leftlevel[MAXSPLT], rightlevel[MAXSPLT];
} sflooper;

static int32_t sflooper_init(CSOUND *csound, sflooper *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;
    int32_t layersNum, j, spltNum = 0;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));

    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];
    if (!preset) {
      return csound->InitError(csound, "%s", Str("sfplay: invalid or "
                                           "out-of-range preset number"));
    }
    layersNum = preset->layers_num;
    for (j =0; j < layersNum; j++) {
      layerType *layer = &preset->layer[j];
      int32_t vel= (int32_t) *p->ivel, notnum= (int32_t) *p->inotnum;
      if (notnum >= layer->minNoteRange &&
          notnum <= layer->maxNoteRange &&
          vel    >= layer->minVelRange  &&
          vel    <= layer->maxVelRange) {
        int32_t splitsNum = layer->splits_num, k;
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
            int32_t orgkey = split->overridingRootKey;
            if (orgkey == -1) orgkey = sample->byOriginalKey;
            orgfreq = globals->pitches[orgkey];

            if (*p->iflag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->freq[spltNum]= (freq/(orgfreq*orgfreq))*
                               sample->dwSampleRate*CS_ONEDSR;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow(2.0, ONETWELTH * (split->scaleTuning*0.01) * (notnum-orgkey));
              p->freq[spltNum]= (freq/orgfreq) * sample->dwSampleRate*CS_ONEDSR;
            }

            attenuation = (MYFLT) (layer->initialAttenuation +
                                   split->initialAttenuation);
            attenuation = POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * attenuation )
              * GLOBAL_ATTENUATION;
            pan = (double)(split->pan + layer->pan) / 1000.0 + 0.5;
            if (pan > 1.0) pan = 1.0;
            else if (pan < 0.0) pan = 0.0;
            p->sBase[spltNum] = sBase;
            p->sstart[spltNum] = start;
            p->end[spltNum] =  (DWORD) (sample->dwEnd + split->endOffset);
            p->leftlevel[spltNum] = (MYFLT) sqrt(1.0-pan) * attenuation;
            p->rightlevel[spltNum] = (MYFLT) sqrt(pan) * attenuation;
            spltNum++;
          }
        }
      }
    }
  p->spltNum = spltNum;
  if (*p->ifn2 != 0) p->efunc = csound->FTFind(csound, p->ifn2);
  else p->efunc = NULL;

  if (*p->iskip == 0){
    p->mode = (int32_t) *p->imode;

    for (j=0; j < spltNum; j++) {
      if (p->mode == 0 || p->mode == 2){
        if ((p->ndx[j][0] = *p->start*CS_ESR+p->sstart[j]) < 0)
          p->ndx[j][0] = 0;
        if (p->ndx[j][0] >= p->end[j])
          p->ndx[j][0] = (double) p->end[j] - 1.0;
        p->count = 0;
      }
      p->firsttime[j] = 1;
    }
    p->init = 1;

  }
  return OK;
}

static int32_t sflooper_process(CSOUND *csound, sflooper *p)
{
    int32_t      k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT    *outL = p->outL, *outR = p->outR, out, sr = CS_ESR;
    MYFLT    amp = *(p->amp), pit = *(p->pitch);
    SHORT    **base = p->sBase, *tab;
    double *ndx;
    MYFLT frac0, frac1, *etab, left, right;
    int32_t *nend = p->end, *loop_end = p->lend, *loop_start = p->lstart,
      crossfade = p->cfade, send, sstart, spltNum = p->spltNum;
    MYFLT count = p->count,fadein, fadeout, pitch;
    int32_t *firsttime = p->firsttime, elen, mode=p->mode, init = p->init;
    uint32 tndx0, tndx1;

    if (p->efunc != NULL) {
      etab = p->efunc->ftable;
      elen = p->efunc->flen;
    }
    else {
      etab = NULL;
      elen = 0;
    }

    /* loop parameters & check */
    if (pit < FL(0.0)) pit = FL(0.0);
    memset(outL, 0, nsmps*sizeof(MYFLT));
    memset(outR, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    for (k=0; k < spltNum; k++) {

      tab   = base[k];
      sstart = p->sstart[k];
      send   = nend[k] + sstart;
      ndx   = p->ndx[k];
      left  = p->leftlevel[k];
      right = p->rightlevel[k];
      pitch = pit*p->freq[k];

      if (firsttime[k]) {
        int32_t loopsize;
        loop_start[k] = (int32_t) (*p->loop_start*sr) + sstart;
        loop_end[k] =   (int32_t) (*p->loop_end*sr) + sstart;
        loop_start[k] = loop_start[k] < sstart ? sstart : loop_start[k];
        /* TODO : CHECKS */
        if(loop_start[k] > send) {
          csound->Warning(csound, "loop start %f beyond sample end %f, clamping.\n",
                          (loop_start[k] - sstart)/sr,
                          (send - sstart)/sr);
          loop_start[k] = send;
        }
        if(loop_end[k] > send) {
          csound->Warning(csound, "loop end %f beyond sample end %f, clamping.\n",
                          (loop_end[k] - sstart)/sr,
                          (send - sstart)/sr);
          loop_end[k] = send;
        }
        loopsize = loop_end[k] - loop_start[k];
        crossfade = (int32_t) (*p->crossfade*sr);
       if (mode == 1) {
          ndx[0] = (double) loop_end[k];
          ndx[1] = (double) loop_end[k];
          count = (MYFLT) crossfade;
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
        }
        else if (mode == 2) {
          ndx[1] = (double) loop_start[k] - 1.0;
          p->cfade = crossfade = crossfade > loopsize/2 ? loopsize/2 - 1 : crossfade;
        }
        else {
          ndx[1] = (double) loop_start[k];
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
        }
        firsttime[k] = 0;
      }
      for (i=offset; i < nsmps; i++) {
        if (mode == 1){ /* backwards */
          tndx0 = (int32_t) ndx[0];
          frac0 = ndx[0] - tndx0;
          if (ndx[0] > crossfade + loop_start[k])
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          else {
            tndx1 = (int32_t) ndx[1];
            frac1 = ndx[1] - tndx1;
            if (etab==NULL){
              fadeout = count/crossfade;
              fadein = FL(1.0) - fadeout;
            }
            else {
              fadeout = elen*count/crossfade;
              fadein = etab[elen - (int32_t)fadeout];
              fadeout = etab[(int32_t)fadeout];
            }
            out = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]))
                      + fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1])));

            ndx[1] -= pitch;
            count -= pitch;
          }
          ndx[0] -= pitch;

          if (ndx[0] <= loop_start[k]) {
            int32_t loopsize;
            loop_start[k] = (int32_t) (*p->loop_start*sr) + sstart;
            loop_end[k] =   (int32_t) (*p->loop_end*sr) + sstart;
            loop_start[k] = loop_start[k] < sstart ? sstart: loop_start[k];
            /* CHECKS */
            if(loop_start[k] > send) {
             csound->Warning(csound, "loop start %f beyond sample end %f, clamping.\n",
                          (loop_start[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_start[k] = send;
            }
            if(loop_end[k] > send) {
              csound->Warning(csound, "loop end %f beyond sample end %f, clamping.\n",
                          (loop_end[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_end[k] = send;
            }
            loopsize = loop_end[k] - loop_start[k];
            crossfade = (int32_t) (*p->crossfade*sr);
            p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
            ndx[0] = ndx[1];
            ndx[1] =  (double)loop_end[k];
            count=(MYFLT)crossfade;
          }
          outR[i] += out*right;
          outL[i] += out*left;
        }
        else if (mode==2) { /* back and forth */
          out = 0;
          /* this is the forward reader */
          if (init && ndx[0] < loop_start[k] + crossfade) {
            tndx0 = (int32_t) ndx[0];
            frac0 = ndx[0] - tndx0;
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
          }
          else if (ndx[0] < loop_start[k] + crossfade) {
            if (etab==NULL) fadein = count/crossfade;
            else fadein = etab[(int32_t)(elen*count/crossfade)];
            tndx0 = (int32_t) ndx[0];
            frac0 = ndx[0] - tndx0;
            out += amp*fadein*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
            count  += pitch;
          }
          else if (ndx[0] < loop_end[k] - crossfade) {
            tndx0 = (int32_t) ndx[0];
            frac0 = ndx[0] - tndx0;
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
            init = 0;
            if (ndx[0] >= loop_end[k] - crossfade) {
              ndx[1] = (double) loop_end[k];
              count = 0;
            }
          }
          else if (ndx[0] < loop_end[k]) {
            if (etab==NULL) fadeout = FL(1.0) - count/crossfade;
            else  fadeout = etab[(int32_t)(elen*(FL(1.0) - count/crossfade))];
            tndx0 = (int32_t) ndx[0];
            frac0 = ndx[0] - tndx0;
            out += amp*fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
            count  += pitch;
          }
          /* this is the backward reader */
          if (ndx[1] > loop_end[k] - crossfade) {
            if (etab==NULL) fadein = count/crossfade;
            else fadein = etab[(int32_t)(elen*count/crossfade)];
            tndx1 = (int32_t) ndx[1];
            frac1 = ndx[1] - tndx1;
            out += amp*fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
            ndx[1] -= pitch;
          }
          else if (ndx[1] > loop_start[k] + crossfade) {
            tndx1 = (int32_t) ndx[1];
            frac1 = ndx[1] - tndx1;
            out = amp*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
            ndx[1] -= pitch;
            if (ndx[1] <= loop_start[k] + crossfade) {
              ndx[0] = (double) loop_start[k];
              count = 0;
            }
          }
          else if (ndx[1] > loop_start[k]) {
            if (etab==NULL) fadeout = FL(1.0) - count/crossfade;
            else fadeout = etab[(int32_t)(elen*(FL(1.0) - count/crossfade))];
            tndx1 = (int32_t) ndx[1];
            frac1 = ndx[1] - tndx1;
            out += amp*fadeout*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
            ndx[1] -= pitch;
            if (ndx[1] <= loop_start[k]) {
              int32_t loopsize;
              loop_start[k] = (int32_t) (*p->loop_start*sr) + p->sstart[k];
              loop_end[k] =   (int32_t) (*p->loop_end*sr) + p->sstart[k];
              loop_start[k] = loop_start[k] < sstart ? sstart: loop_start[k];
                          /* CHECKS */
              if(loop_start[k] > send) {
               csound->Warning(csound, "loop start %f beyond sample end %f, clamping.\n",
                          (loop_start[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_start[k] = send;
            }
            if(loop_end[k] > send) {
              csound->Warning(csound, "loop end %f beyond sample end %f, clamping.\n",
                          (loop_end[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_end[k] = send;
            }

              loopsize = loop_end[k] - loop_start[k];
              crossfade = (int32_t) (*p->crossfade*sr);
              p->cfade = crossfade =
                crossfade > loopsize/2 ? loopsize/2-1 : crossfade;
            }
          }
          outR[i] += out*right;
          outL[i] += out*left;
        }
        else {  /* normal */
          //out = 0;
          tndx0 = (uint32) ndx[0];
          frac0 = ndx[0] - tndx0;
          if (ndx[0] < loop_end[k]-crossfade)
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          else {
            tndx1 = (int32_t) ndx[1];
            frac1 = ndx[1] - tndx1;
            if (etab==NULL) {
              fadein = count/crossfade;
              fadeout = FL(1.0) - fadein;
            }
            else {
              fadein = elen*count/crossfade;
              fadeout = etab[elen - (int32_t)fadein];
              fadein = etab[(int32_t)fadein];
          }
            out = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]))
                       + fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1])));
            ndx[1]+=pitch;
            count+=pitch;
          }
          ndx[0]+=pitch;
          if (ndx[0] >= loop_end[k]) {
            int32_t loopsize;
            loop_start[k] = (int32_t) (*p->loop_start*sr) + p->sstart[k];
            loop_end[k] =   (int32_t) (*p->loop_end*sr) + p->sstart[k];
            loop_start[k] = loop_start[k] < sstart ? sstart: loop_start[k];
            /* TODO : CHECKS */
            if(loop_start[k] > send) {
             csound->Warning(csound, "loop start %f beyond sample end %f, clamping.\n",
                          (loop_start[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_start[k] = send;
            }
            if(loop_end[k] > send) {
              csound->Warning(csound, "loop end %f beyond sample end %f, clamping.\n",
                          (loop_end[k] - sstart)/sr,
                          (send - sstart)/sr);
              loop_end[k] = send;
            }
            loopsize = loop_end[k] - loop_start[k];
            crossfade = (int32_t) (*p->crossfade*sr);
            p->cfade = crossfade = crossfade > loopsize ? loopsize-1 : crossfade;
            ndx[0] = ndx[1];
            ndx[1] = (double)loop_start[k];
            count=0;
          }
          outR[i] += out*right;
          outL[i] += out*left;
        }
      }

    }
    p->count = count;
    p->cfade = crossfade;

    p->init = init;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
  { "sfload",S(SFLOAD),     0,    "i",    "S",      (SUBR)SfLoad_S, NULL, NULL },
  { "sfload.i",S(SFLOAD),     0,    "i",    "i",   (SUBR)SfLoad, NULL, NULL },
  { "sfpreset",S(SFPRESET), 0,    "i",    "iiii",   (SUBR)SfPreset         },
  { "sfplay", S(SFPLAY), 0,  "aa", "iixxioooo",
    (SUBR)SfPlay_set, (SUBR)SfPlay     },
  { "sfplaym", S(SFPLAYMONO), 0,  "a", "iixxioooo",
    (SUBR)SfPlayMono_set, (SUBR)SfPlayMono },
  { "sfplist",S(SFPLIST),   0,    "",     "i",      (SUBR)Sfplist          },
  { "sfilist",S(SFPLIST),   0,    "",     "i",      (SUBR)Sfilist          },
  { "sfilist.prefix",S(SFPLIST),   0,    "",     "iS",      (SUBR)Sfilist_prefix},

  { "sfpassign",S(SFPASSIGN), 0,  "",     "iip",    (SUBR)SfAssignAllPresets },
  { "sfinstrm", S(SFIPLAYMONO),0, "a", "iixxiioooo",
    (SUBR)SfInstrPlayMono_set, (SUBR)SfInstrPlayMono },
  { "sfinstr", S(SFIPLAY),  0,    "aa", "iixxiioooo",
    (SUBR)SfInstrPlay_set,(SUBR)SfInstrPlay },
  { "sfplay3", S(SFPLAY),   0,    "aa", "iixxioooo",
    (SUBR)SfPlay_set, (SUBR)SfPlay3  },
  { "sfplay3m", S(SFPLAYMONO), 0, "a", "iixxioooo",
    (SUBR)SfPlayMono_set,(SUBR)SfPlayMono3 },
  { "sfinstr3", S(SFIPLAY), 0,    "aa", "iixxiioooo",
    (SUBR)SfInstrPlay_set, (SUBR)SfInstrPlay3 },
  { "sfinstr3m", S(SFIPLAYMONO), 0, "a", "iixxiioooo",
    (SUBR)SfInstrPlayMono_set, (SUBR)SfInstrPlayMono3 },
  { "sflooper", S(sflooper), 0, "aa", "iikkikkkooooo",
    (SUBR)sflooper_init, (SUBR)sflooper_process },
  { NULL, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
};

int32_t sfont_ModuleCreate(CSOUND *csound)
{
    int32_t j;
    sfontg *globals;
    csound->CreateGlobalVariable(csound, "::sfontg",
                                 sizeof(sfontg));
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (globals == NULL)
      return csound->InitError(csound,
                               "%s", Str("error... could not create sfont globals\n"));

    globals->sfArray = (SFBANK *) csound->Calloc(csound, MAX_SFONT*sizeof(SFBANK));
    globals->presetp =
      (presetType **) csound->Calloc(csound, MAX_SFPRESET *sizeof(presetType *));
    globals->sampleBase =
      (SHORT **) csound->Calloc(csound, MAX_SFPRESET*sizeof(SHORT *));
    globals->currSFndx = 0;
    globals->maxSFndx = MAX_SFONT;
    for (j=0; j<128; j++) {
      globals->pitches[j] = (MYFLT) (csound->GetA4(csound) * pow(2.0, (double)(j- 69)/12.0));
    }

   return OK;
}

int32_t sfont_ModuleInit(CSOUND *csound)
{
    OENTRY  *ep = (OENTRY*) &(localops[0]);
    int32_t     err = 0;

    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, ep->flags,
                                  ep->outypes, ep->intypes,
                                  (int32_t (*)(CSOUND *, void*)) ep->init,
                                  (int32_t (*)(CSOUND *, void*)) ep->perf,
                                  (int32_t
                                   (*)(CSOUND *, void*)) ep->deinit);
      ep++;
    }
    return err;
}

int32_t sfont_ModuleDestroy(CSOUND *csound)
{
    int32_t j,k,l;
    SFBANK *sfArray;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (globals == NULL) return 0;
    sfArray = globals->sfArray;

    for (j=0; j<globals->currSFndx; j++) {
      for (k=0; k< sfArray[j].presets_num; k++) {
        for (l=0; l<sfArray[j].preset[k].layers_num; l++) {
          csound->Free(csound, sfArray[j].preset[k].layer[l].split);
        }
        csound->Free(csound, sfArray[j].preset[k].layer);
      }
      csound->Free(csound, sfArray[j].preset);
      for (l=0; l< sfArray[j].instrs_num; l++) {
        csound->Free(csound, sfArray[j].instr[l].split);
      }
      csound->Free(csound, sfArray[j].instr);
      csound->Free(csound, sfArray[j].chunk.main_chunk.ckDATA);
    }
    csound->Free(csound, sfArray);
    globals->currSFndx = 0;
    csound->Free(csound, globals->presetp);
    csound->Free(csound, globals->sampleBase);

    csound->DestroyGlobalVariable(csound, "::sfontg");
    return 0;
}


#ifdef BUILD_PLUGINS

PUBLIC int32_t csoundModuleCreate(CSOUND *csound){
  return sfont_ModuleCreate(csound);
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound){
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  return sfont_ModuleDestroy(csound);
}

PUBLIC int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t
) sizeof(MYFLT));
}
#endif
