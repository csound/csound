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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include "sfenum.h"
#include "sfont.h"

#define s2d(x)  *((DWORD *) (x))



static int chunk_read(FILE *f, CHUNK *chunk);
static void fill_SfPointers(CSOUND *);
static void fill_SfStruct(CSOUND *);
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
  int currSFndx;
  int maxSFndx;
  presetType **presetp;
  SHORT **sampleBase;
  MYFLT pitches[128];
} sfontg;

int sfont_ModuleDestroy(CSOUND *csound)
{
    int j,k,l;
    SFBANK *sfArray;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (globals == NULL) return 0;
    sfArray = globals->sfArray;

    for (j=0; j<globals->currSFndx; j++) {
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
    free(sfArray);
    globals->currSFndx = 0;
    csound->Free(csound, globals->presetp);
    csound->Free(csound, globals->sampleBase);

    csound->DestroyGlobalVariable(csound, "::sfontg");
    return 0;
}

static void SoundFontLoad(CSOUND *csound, char *fname)
{
    FILE *fil;
    void *fd;
    SFBANK *soundFont;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    soundFont = globals->soundFont;
    fd = csound->FileOpen2(csound, &fil, CSFILE_STD, fname, "rb",
                             "SFDIR;SSDIR", CSFTYPE_SOUNDFONT, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->ErrorMsg(csound,
                  Str("sfload: cannot open SoundFont file \"%s\" (error %s)"),
                  fname, strerror(errno));
      return;
    }
    soundFont = &globals->sfArray[globals->currSFndx];
    if (UNLIKELY(soundFont==NULL)){
      csound->ErrorMsg(csound, Str("Sfload: cannot use globals"));
      return;
    }
    strncpy(soundFont->name, csound->GetFileName(fd), 255);
    soundFont->name[255]='\0';
    if (UNLIKELY(chunk_read(fil, &soundFont->chunk.main_chunk)<0))
      csound->Message(csound, Str("sfont: failed to read file\n"));
    csound->FileClose(csound, fd);
    globals->soundFont = soundFont;
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

static int SfLoad_(CSOUND *csound, SFLOAD *p, int istring)
                                       /* open a file and return its handle */
{                                      /* the handle is simply a stack index */
    char *fname;
    SFBANK *sf;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (UNLIKELY(globals==NULL)) {
      return csound->InitError(csound, Str("sfload: could not open globals\n"));
    }
    if(istring) fname = csound->Strdup(csound, ((STRINGDAT *)p->fname)->data);
    else {
      if(ISSTRCOD(*p->fname))
        fname = csound->Strdup(csound, get_arg_string(csound,*p->fname));
      else fname = csound->strarg2name(csound,
                                NULL, p->fname, "sfont.",
                                0);
    }
    /*    strcpy(fname, (char*) p->fname); */
    Gfname = fname;
    SoundFontLoad(csound, fname);
    *p->ihandle = (float) globals->currSFndx;
    sf = &globals->sfArray[globals->currSFndx];
    qsort(sf->preset, sf->presets_num, sizeof(presetType),
        (int (*)(const void *, const void * )) compare);
    csound->Free(csound,fname);
    if (UNLIKELY(++globals->currSFndx>=globals->maxSFndx)) {
      globals->maxSFndx += 5;
      globals->sfArray = (SFBANK *)realloc(globals->sfArray,
                                           globals->maxSFndx*sizeof(SFBANK));
      csound->Warning(csound, Str("Extending soundfonts"));
    }
    return OK;
}

static int SfLoad(CSOUND *csound, SFLOAD *p){
  return SfLoad_(csound,p,0);
}

static int SfLoad_S(CSOUND *csound, SFLOAD *p){
  return SfLoad_(csound,p,1);
}

static char *filter_string(char *s, char temp_string[24])
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

static int Sfplist(CSOUND *csound, SFPLIST *p)
{
    sfontg *globals;
    SFBANK *sf;
    char temp_string[24];
    int j;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[(int) *p->ihandle];
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

static int SfAssignAllPresets(CSOUND *csound, SFPASSIGN *p)
{
    sfontg *globals;
    SFBANK *sf;
    int pHandle, pnum;
    int j, enableMsgs;

    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[(int) *p->ihandle];
    pHandle = (int) *p->startNum;
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
                                  " handles from %d to %d \n\n"),
                              (int) *p->startNum, pHandle - 1);
    return OK;
}

static int Sfilist(CSOUND *csound, SFPLIST *p)
{
    sfontg *globals;
    SFBANK *sf;
    int j;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[(int) *p->ihandle];
    csound->Message(csound, Str("\nInstrument list of \"%s\"\n"), sf->name);
    for (j =0; j < sf->instrs_num; j++) {
      instrType *inst = &sf->instr[j];
      csound->Message(csound, "%3d) %-20s\n", j, inst->name);
    }
    csound->Message(csound, "\n");
    return OK;
}

static int SfPreset(CSOUND *csound, SFPRESET *p)
{
    sfontg *globals; SFBANK *sf;
    int j, presetHandle = (int) *p->iPresetHandle;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[(DWORD) *p->isfhandle];

    if (presetHandle >= MAX_SFPRESET) {
      return csound->InitError(csound,
                               Str("sfpreset: preset handle too big (%d), max: %d"),
                               presetHandle, (int) MAX_SFPRESET - 1);
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
      return csound->InitError(csound,
                               Str("sfpreset: cannot find any preset having prog "
                                   "number %d and bank number %d in SoundFont file"
                                   " \"%s\""),
                               (int) *p->iprog, (int) *p->ibank,
                               globals->sfArray[(DWORD) *p->isfhandle].name);
    }
    return OK;
}

static int SfPlay_set(CSOUND *csound, SFPLAY *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;

    int layersNum, j, spltNum = 0, flag = (int) *p->iflag;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];

    if (!UNLIKELY(preset!=NULL)) {
      return csound->InitError(csound, Str("sfplay: invalid or "
                                           "out-of-range preset number"));
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
            orgfreq = globals->pitches[orgkey];
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*
                               sample->dwSampleRate*csound->onedsr;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow(2.0, ONETWELTH * (split->scaleTuning*0.01) * (notnum-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*csound->onedsr;
            }
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
            p->end[spltNum] = sample->dwEnd + split->endOffset - start;
            p->startloop[spltNum] =
              sample->dwStartloop + split->startLoopOffset  - start;
            p->endloop[spltNum] =
              sample->dwEndloop + split->endLoopOffset - start;
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
        int   x0 = (int32)phs1 ;\
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

static int SfPlay(CSOUND *csound, SFPLAY *p)
{
    MYFLT   *out1 = p->out1, *out2 = p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop= p->startloop, *endloop= p->endloop,
          *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
      *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
      *release = p->release, *attr = p->attr;


    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfPlay3(CSOUND *csound, SFPLAY *p)
{
    MYFLT    *out1 = p->out1, *out2 = p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end = p->end,  *startloop = p->startloop,
          *endloop = p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
          *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
          *release = p->release, *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;
/*         nsmps = CS_KSMPS; */
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while(j--) {
        double looplength = *endloop - *startloop, si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfPlayMono_set(CSOUND *csound, SFPLAYMONO *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;
    /* int layersNum= preset->layers_num, j, spltNum = 0, flag=(int) *p->iflag; */

    int layersNum, j, spltNum = 0, flag=(int) *p->iflag;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];

    if (UNLIKELY(!preset)) {
      return csound->InitError(csound, Str("sfplaym: invalid or "
                                           "out-of-range preset number"));
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
            orgfreq = globals->pitches[orgkey] ;
            if (flag) {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
              p->si[spltNum]= (freq/(orgfreq*orgfreq))*
                               sample->dwSampleRate*csound->onedsr;
            }
            else {
              freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (notnum-orgkey));
              p->si[spltNum]= (freq/orgfreq) * sample->dwSampleRate*csound->onedsr;
            }
            p->attenuation[spltNum] =
              POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * (layer->initialAttenuation +
                                                    split->initialAttenuation)) *
              GLOBAL_ATTENUATION;
            p->base[spltNum] =  sBase+ start;
            p->phs[spltNum] = (double) split->startOffset + *p->ioffset;
            p->end[spltNum] = sample->dwEnd + split->endOffset - start;
            p->startloop[spltNum] = sample->dwStartloop +
              split->startLoopOffset - start;
            p->endloop[spltNum] = sample->dwEndloop + split->endLoopOffset - start;
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

static int SfPlayMono(CSOUND *csound, SFPLAYMONO *p)
{
    MYFLT   *out1 = p->out1 , *env  = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end, *startloop= p->startloop, *endloop= p->endloop,
          *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation, *attack = p->attack, *decr = p->decr,
          *decay = p->decay, *sustain= p->sustain, *release = p->release,
          *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfPlayMono3(CSOUND *csound, SFPLAYMONO *p)
{
    MYFLT   *out1 = p->out1, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT  **base = p->base;
    DWORD   *end = p->end,  *startloop = p->startloop,
            *endloop = p->endloop, *tinc = p->ti;
    SHORT   *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation,*attack = p->attack, *decr = p->decr,
          *decay = p->decay, *sustain= p->sustain, *release = p->release,
          *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;
    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfInstrPlay_set(CSOUND *csound, SFIPLAY *p)
{
    sfontg *globals;
    SFBANK *sf;
    int index = (int) *p->sfBank;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[index];
    if (UNLIKELY(index > globals->currSFndx || *p->instrNum >  sf->instrs_num)) {
      return csound->InitError(csound, Str("sfinstr: instrument out of range"));
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
          orgfreq = globals->pitches[orgkey] ;
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*
                              sample->dwSampleRate*csound->onedsr;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01)*(notnum - orgkey));
            p->si[spltNum] = (freq/orgfreq)*(sample->dwSampleRate*csound->onedsr);
          }
          attenuation = (MYFLT) (split->initialAttenuation);
          attenuation = POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * attenuation) *
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

static int SfInstrPlay(CSOUND *csound, SFIPLAY *p)
{
    MYFLT *out1= p->out1, *out2= p->out2, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel, *attack = p->attack,
          *decr = p->decr, *decay = p->decay, *sustain= p->sustain,
          *release = p->release, *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfInstrPlay3(CSOUND *csound, SFIPLAY *p)
{
    MYFLT *out1= p->out1, *out2= p->out2,*env =p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *left= p->leftlevel, *right= p->rightlevel,
      *attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, *release = p->release,
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    memset(out2, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfInstrPlayMono_set(CSOUND *csound, SFIPLAYMONO *p)
{
    int index = (int) *p->sfBank;
    sfontg *globals;
    SFBANK *sf;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    sf = &globals->sfArray[index];
    if (UNLIKELY(index > globals->currSFndx || *p->instrNum >  sf->instrs_num)) {
      return csound->InitError(csound, Str("sfinstr: instrument out of range"));
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
          orgfreq = globals->pitches[orgkey];
          if (flag) {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection);
            p->si[spltNum] = (freq/(orgfreq*orgfreq))*
                              sample->dwSampleRate*csound->onedsr;
          }
          else {
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection)
              * pow( 2.0, ONETWELTH* (split->scaleTuning*0.01) * (notnum-orgkey));
            p->si[spltNum] = (freq/orgfreq)*(sample->dwSampleRate*csound->onedsr);
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

static int SfInstrPlayMono(CSOUND *csound, SFIPLAYMONO *p)
{
    MYFLT *out1= p->out1, *env = p->env;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
     uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop, *endloop= p->endloop,
      *tinc = p->ti;
    SHORT *mode = p->mode;

    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation, *attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, *release = p->release,
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static int SfInstrPlayMono3(CSOUND *csound, SFIPLAYMONO *p)
{
    MYFLT *out1= p->out1, *env = p->env  ;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      j = p->spltNum;
    SHORT **base = p->base;
    DWORD *end= p->end,  *startloop= p->startloop,
          *endloop= p->endloop, *tinc = p->ti;
    SHORT *mode = p->mode;
    double *sampinc = p->si, *phs = p->phs;
    MYFLT *attenuation = p->attenuation,*attack = p->attack, *decr = p->decr,
      *decay = p->decay, *sustain= p->sustain, *release = p->release,
      *attr = p->attr;

    memset(out1, 0, nsmps*sizeof(MYFLT));
    if (UNLIKELY(early)) nsmps -= early;

    if (IS_ASIG_ARG(p->xfreq)) {
      while (j--) {
        double looplength = *endloop - *startloop;
        MYFLT *freq = p->xfreq;

        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
      }
    }
    else {
      MYFLT freq = *p->xfreq;
      while (j--) {
        double looplength = *endloop - *startloop;
        double si = *sampinc * freq;
        if (*mode == 1 || *mode ==3) {
          int flag =0;
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
        release++; tinc++; env++; attr++; decr++;
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

static void fill_SfStruct(CSOUND *csound)
{
    int j, k, i, l, m, size, iStart, iEnd, kk, ll, mStart, mEnd;
    int pbag_num,first_pbag,layer_num;
    int ibag_num,first_ibag,split_num;
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
    preset = (presetType *) malloc(size * sizeof(presetType));
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
                  split->attack = split->decay = split->sustain =
                    split->release = FL(0.0);
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
                        if (UNLIKELY(split->sample->sfSampleType & 0x8000)) {
                          free(preset);
                          csound->ErrorMsg(csound, Str("SoundFont file \"%s\" "
                                                       "contains ROM samples !\n"
                                                       "At present time only RAM "
                                                       "samples are allowed "
                                                       "by sfload.\n"
                                                       "Session aborted !"),
                                           Gfname);
                            return;
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
                    case delayVolEnv:
                      csound->Message(csound, "del: %f\n",
                                      (double) igen[m].genAmount.shAmount);
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
                  if (UNLIKELY(split->sample->sfSampleType & 0x8000)) {
                    free(instru);
                    csound->ErrorMsg(csound, Str("SoundFont file \"%s\" contains "
                                            "ROM samples !\n"
                                            "At present time only RAM samples "
                                            "are allowed by sfload.\n"
                                                 "Session aborted !"), Gfname);
                    return;
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

static int chunk_read(FILE *fil, CHUNK *chunk)
{
    if (UNLIKELY(4 != fread(chunk->ckID,1,4, fil)))
      return 0;
    if (UNLIKELY(1 != fread(&chunk->ckSize,4,1,fil)))
      return 0;
    ChangeByteOrder("d", (char *)&chunk->ckSize, 4);
    chunk->ckDATA = (BYTE *) malloc( chunk->ckSize);
    return fread(chunk->ckDATA,1,chunk->ckSize,fil);
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
      csound->ErrorMsg(csound, Str("Sfont: cannot use globals/"));
      return;
    }

    soundFont = globals->soundFont;
    if (LIKELY(soundFont != NULL))
      main_chunk=&(soundFont->chunk.main_chunk);
    else  {
     csound->ErrorMsg(csound, Str("Sfont: cannot use globals/"));
     return;
    }

    if (UNLIKELY(main_chunk->ckDATA == NULL)) {
      csound->ErrorMsg(csound, Str("Sfont format not compatible"));
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
/*                                 (char*) &chkid, (unsigned int) size); */
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
/*                                       (char*) &chkid, (unsigned int) chkid); */
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
/*                                   (char*) &chkid, (unsigned int) chkid); */
/* #endif */
          shdrChunk = (CHUNK *) chkp;
          chkp += shdrChunk->ckSize+8;
          j += shdrChunk->ckSize+8;
        }
      }
      else {
/* #ifdef BETA */
/*         csound->Message(csound, "Unknown sfont %.4s(%.8x)\n", */
/*                                 (char*) &chkid, (unsigned int) chkid); */
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
    *crossfade, *start, *imode, *ifn2, *iskip;
  int     spltNum;
  SHORT   *sBase[MAXSPLT];
  FUNC *efunc;
  MYFLT count;
  int lstart[MAXSPLT], lend[MAXSPLT], cfade, mode;
  double  ndx[MAXSPLT][2];    /* table lookup ndx */
  double  freq[MAXSPLT];
  int firsttime, init, end[MAXSPLT], sstart[MAXSPLT];
  MYFLT   leftlevel[MAXSPLT], rightlevel[MAXSPLT];
} sflooper;

static int sflooper_init(CSOUND *csound, sflooper *p)
{
    DWORD index = (DWORD) *p->ipresethandle;
    presetType *preset;
    SHORT *sBase;
    int layersNum, j, spltNum = 0;
    sfontg *globals;
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    preset = globals->presetp[index];
    sBase = globals->sampleBase[index];
    if (!preset) {
      return csound->InitError(csound, Str("sfplay: invalid or "
                                           "out-of-range preset number"));
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
            orgfreq = globals->pitches[orgkey];
            freq = orgfreq * pow(2.0, ONETWELTH * tuneCorrection) *
                pow(2.0, ONETWELTH * (split->scaleTuning*0.01) * (notnum-orgkey));
            p->freq[spltNum]= (freq/orgfreq) * sample->dwSampleRate*csound->onedsr;
            attenuation = (MYFLT) (layer->initialAttenuation +
                                   split->initialAttenuation);
            attenuation = POWER(FL(2.0), (-FL(1.0)/FL(60.0)) * attenuation )
              * GLOBAL_ATTENUATION;
            pan = (double)(split->pan + layer->pan) / 1000.0 + 0.5;
            if (pan > 1.0) pan = 1.0;
            else if (pan < 0.0) pan = 0.0;
            p->sBase[spltNum] = sBase;
            p->sstart[spltNum] = start;
            p->end[spltNum] = sample->dwEnd + split->endOffset;
            p->leftlevel[spltNum] = (MYFLT) sqrt(1.0-pan) * attenuation;
            p->rightlevel[spltNum] = (MYFLT) sqrt(pan) * attenuation;
            spltNum++;
          }
        }
      }
    }
  p->spltNum = spltNum;
  if (*p->ifn2 != 0) p->efunc = csound->FTnp2Find(csound, p->ifn2);
  else p->efunc = NULL;

  if (*p->iskip == 0){
    p->mode = (int) *p->imode;

    for(j=0; j < spltNum; j++){
      if (p->mode == 0 || p->mode == 2){
        if ((p->ndx[j][0] = *p->start*CS_ESR+p->sstart[j]) < 0)
          p->ndx[j][0] = 0;
        if (p->ndx[j][0] >= p->end[j])
          p->ndx[j][0] = (double) p->end[j] - 1.0;
        p->count = 0;
      }
    }
    p->init = 1;
    p->firsttime = 1;
  }
  return OK;
}

static int sflooper_process(CSOUND *csound, sflooper *p)
{
    int      k;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT    *outL = p->outL, *outR = p->outR, out, sr = CS_ESR;
    MYFLT    amp = *(p->amp), pit = *(p->pitch);
    SHORT    **base = p->sBase, *tab;
    double *ndx;
    MYFLT frac0, frac1, *etab, left, right;
    int *nend = p->end, *loop_end = p->lend, *loop_start = p->lstart,
        crossfade = p->cfade, len, spltNum = p->spltNum;
    MYFLT count = p->count,fadein, fadeout, pitch;
    int *firsttime = &p->firsttime, elen, mode=p->mode, init = p->init;
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

    for(k=0; k < spltNum; k++){

      tab = base[k];
      len = nend[k];
      ndx = p->ndx[k];
      left = p->leftlevel[k];
      right = p->rightlevel[k];
      pitch = pit*p->freq[k];

      if (*firsttime) {
        int loopsize;
        loop_start[k] = (int) (*p->loop_start*sr) + p->sstart[k];
        loop_end[k] =   (int) (*p->loop_end*sr) + p->sstart[k];
        loop_start[k] = loop_start[k] < 0 ? 0 : loop_start[k];
        loop_end[k] =   loop_end[k] > len ? len :
          (loop_end[k] < loop_start[k] ? loop_start[k] : loop_end[k]);
        loopsize = loop_end[k] - loop_start[k];
        crossfade = (int) (*p->crossfade*sr);

        if (mode == 1) {
          ndx[0] = (double) loop_end[k];
          ndx[1] = (double) loop_end[k];
          count = (MYFLT) crossfade;
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
        }
        else if (mode == 2) {
          ndx[1] = (double) loop_start[k] - 1.0;
          p->cfade = crossfade = crossfade > loopsize/2 ? loopsize/2-1 : crossfade;
        }
        else {
          ndx[1] = (double) loop_start[k];
          p->cfade = crossfade = crossfade > loopsize ? loopsize : crossfade;
        }
        *firsttime = 0;
      }
      for (i=offset; i < nsmps; i++) {
        if (mode == 1){ /* backwards */
          tndx0 = (int) ndx[0];
          frac0 = ndx[0] - tndx0;
          if (ndx[0] > crossfade + loop_start[k])
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          else {
            tndx1 = (int) ndx[1];
            frac1 = ndx[1] - tndx1;
            if (etab==NULL){
              fadeout = count/crossfade;
              fadein = FL(1.0) - fadeout;
            }
            else {
              fadeout = elen*count/crossfade;
              fadein = etab[elen - (int)fadeout];
              fadeout = etab[(int)fadeout];
            }
            out = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]))
                       + fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1])));

            ndx[1] -= pitch;
            count -= pitch;
          }
          ndx[0] -= pitch;

          if (ndx[0] <= loop_start[k]) {
            int loopsize;
            loop_start[k] = (int) (*p->loop_start*sr) + p->sstart[k];
            loop_end[k] =   (int) (*p->loop_end*sr) + p->sstart[k];
            loop_start[k] = loop_start[k] < 0 ? 0 : loop_start[k];
            loop_end[k] =   loop_end[k] > len ? len :
              (loop_end[k] < loop_start[k] ? loop_start[k] : loop_end[k]);
            loopsize = loop_end[k] - loop_start[k];
            crossfade = (int) (*p->crossfade*sr);
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
            tndx0 = (int) ndx[0];
            frac0 = ndx[0] - tndx0;
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
          }
          else if (ndx[0] < loop_start[k] + crossfade) {
            if (etab==NULL) fadein = count/crossfade;
            else fadein = etab[(int)(elen*count/crossfade)];
            tndx0 = (int) ndx[0];
            frac0 = ndx[0] - tndx0;
            out += amp*fadein*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
            count  += pitch;
          }
          else if (ndx[0] < loop_end[k] - crossfade) {
            tndx0 = (int) ndx[0];
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
            else  fadeout = etab[(int)(elen*(FL(1.0) - count/crossfade))];
            tndx0 = (int) ndx[0];
            frac0 = ndx[0] - tndx0;
            out += amp*fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
            ndx[0] += pitch;
            count  += pitch;
          }
          /* this is the backward reader */
          if (ndx[1] > loop_end[k] - crossfade) {
            if (etab==NULL) fadein = count/crossfade;
            else fadein = etab[(int)(elen*count/crossfade)];
            tndx1 = (int) ndx[1];
            frac1 = ndx[1] - tndx1;
            out += amp*fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
            ndx[1] -= pitch;
          }
          else if (ndx[1] > loop_start[k] + crossfade) {
            tndx1 = (int) ndx[1];
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
            else fadeout = etab[(int)(elen*(FL(1.0) - count/crossfade))];
            tndx1 = (int) ndx[1];
            frac1 = ndx[1] - tndx1;
            out += amp*fadeout*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1]));
            ndx[1] -= pitch;
            if (ndx[1] <= loop_start[k]) {
              int loopsize;
              loop_start[k] = (int) (*p->loop_start*sr) + p->sstart[k];
              loop_end[k] =   (int) (*p->loop_end*sr) + p->sstart[k];
              loop_start[k] = loop_start[k] < 0 ? 0 : loop_start[k];
              loop_end[k] =   loop_end[k] > len ? len :
                (loop_end[k] < loop_start[k] ? loop_start[k] : loop_end[k]);
              loopsize = loop_end[k] - loop_start[k];
              crossfade = (int) (*p->crossfade*sr);
              p->cfade = crossfade =
                crossfade > loopsize/2 ? loopsize/2-1 : crossfade;
            }
          }
          outR[i] += out*right;
          outL[i] += out*left;
        }
        else {  /* normal */
          out = 0;
          tndx0 = (uint32) ndx[0];
          frac0 = ndx[0] - tndx0;
          if (ndx[0] < loop_end[k]-crossfade)
            out = amp*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]));
          else {
            tndx1 = (int) ndx[1];
            frac1 = ndx[1] - tndx1;
            if (etab==NULL) {
              fadein = count/crossfade;
              fadeout = FL(1.0) - fadein;
            }
            else {
              fadein = elen*count/crossfade;
              fadeout = etab[elen - (int)fadein];
              fadein = etab[(int)fadein];
          }
            out = amp*(fadeout*(tab[tndx0] + frac0*(tab[tndx0+1] - tab[tndx0]))
                       + fadein*(tab[tndx1] + frac1*(tab[tndx1+1] - tab[tndx1])));
            ndx[1]+=pitch;
            count+=pitch;
          }
          ndx[0]+=pitch;
          if (ndx[0] >= loop_end[k]) {
            int loopsize;
            loop_start[k] = (int) (*p->loop_start*sr) + p->sstart[k];
            loop_end[k] =   (int) (*p->loop_end*sr) + p->sstart[k];
            loop_start[k] = loop_start[k] < 0 ? 0 : loop_start[k];
            loop_end[k] =   loop_end[k] > len ? len :
              (loop_end[k] < loop_start[k] ? loop_start[k] : loop_end[k]);
            loopsize = loop_end[k] - loop_start[k];
            crossfade = (int) (*p->crossfade*sr);
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
  { "sfload",S(SFLOAD),     0, 1,    "i",    "S",      (SUBR)SfLoad_S, NULL, NULL },
   { "sfload.i",S(SFLOAD),     0, 1,    "i",    "i",   (SUBR)SfLoad, NULL, NULL },
  { "sfpreset",S(SFPRESET), 0, 1,    "i",    "iiii",   (SUBR)SfPreset         },
  { "sfplay", S(SFPLAY), 0, 5, "aa", "iixxiooo",       (SUBR)SfPlay_set,
                                                       NULL, (SUBR)SfPlay     },
  { "sfplaym", S(SFPLAYMONO), 0, 5, "a", "iixxiooo",    (SUBR)SfPlayMono_set,
                                                       NULL, (SUBR)SfPlayMono },
  { "sfplist",S(SFPLIST),   0, 1,    "",     "i",      (SUBR)Sfplist          },
  { "sfilist",S(SFPLIST),   0, 1,    "",     "i",      (SUBR)Sfilist          },
  { "sfpassign",S(SFPASSIGN), 0, 1,  "",     "iip",    (SUBR)SfAssignAllPresets },
  { "sfinstrm", S(SFIPLAYMONO),0, 5, "a", "iixxiiooo", (SUBR)SfInstrPlayMono_set,
                                                  NULL, (SUBR)SfInstrPlayMono },
  { "sfinstr", S(SFIPLAY),  0, 5,    "aa", "iixxiiooo", (SUBR)SfInstrPlay_set,
                                                       NULL,(SUBR)SfInstrPlay },
  { "sfplay3", S(SFPLAY),   0, 5,    "aa", "iixxiooo",  (SUBR)SfPlay_set,
                                                    NULL, (SUBR)SfPlay3  },
  { "sfplay3m", S(SFPLAYMONO), 0, 5, "a", "iixxiooo",   (SUBR)SfPlayMono_set,
                                                    NULL,(SUBR)SfPlayMono3 },
  { "sfinstr3", S(SFIPLAY), 0, 5,    "aa", "iixxiiooo", (SUBR)SfInstrPlay_set,
                                                    NULL, (SUBR)SfInstrPlay3 },
  { "sfinstr3m", S(SFIPLAYMONO), 0, 5, "a", "iixxiiooo",(SUBR)SfInstrPlayMono_set,
                                                    NULL, (SUBR)SfInstrPlayMono3 },
  { "sflooper", S(sflooper), 0, 5, "aa", "iikkikkkoooo",  (SUBR)sflooper_init,
                                                    NULL, (SUBR)sflooper_process },
  { NULL, 0, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
};

int sfont_ModuleCreate(CSOUND *csound)
{
    int j;
    sfontg *globals;
    csound->CreateGlobalVariable(csound, "::sfontg",
                                 sizeof(sfontg));
    globals = (sfontg *) (csound->QueryGlobalVariable(csound, "::sfontg"));
    if (globals == NULL)
      return csound->InitError(csound,
                               Str("error... could not create sfont globals\n"));

    globals->sfArray = (SFBANK *) malloc(MAX_SFONT*sizeof(SFBANK));
    globals->presetp =
      (presetType **) csound->Malloc(csound, MAX_SFPRESET *sizeof(presetType *));
    globals->sampleBase =
      (SHORT **) csound->Malloc(csound, MAX_SFPRESET*sizeof(SHORT *));
    globals->currSFndx = 0;
    globals->maxSFndx = MAX_SFONT;
    for (j=0; j<128; j++) {
      globals->pitches[j] = (MYFLT) (440.0 * pow(2.0, (double)(j- 69)/12.0));
    }

   return OK;
}

int sfont_ModuleInit(CSOUND *csound)
{
    OENTRY  *ep = (OENTRY*) &(localops[0]);
    int     err = 0;

    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, ep->flags,
                                  ep->thread, ep->outypes, ep->intypes,
                                  (int (*)(CSOUND *, void*)) ep->iopadr,
                                  (int (*)(CSOUND *, void*)) ep->kopadr,
                                  (int (*)(CSOUND *, void*)) ep->aopadr);
      ep++;
    }
    return err;
}
