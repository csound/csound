/*
bus.c:

 Copyright (C) 2004 John ffitch
 (C) 2005, 2006 Istvan Varga
 (c) 2006 Victor Lazzarini

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
/*                      BUS.C           */
#include "csoundCore.h"
#include <setjmp.h>
#include <ctype.h>

#define CSOUND_BUS_C  1

#include "aops.h"
#include "bus.h"
#include "namedins.h"

static CS_NOINLINE int chan_realloc(CSOUND *csound,
                                    MYFLT **p, int *oldSize, int newSize)
{
    volatile jmp_buf  saved_exitjmp;
    MYFLT             *newp;
    int               i;

    memcpy((void*) &saved_exitjmp, (void*)&csound->exitjmp, sizeof(jmp_buf));
    if (setjmp(csound->exitjmp) != 0) {
      memcpy((void*)&csound->exitjmp, (void*)&saved_exitjmp, sizeof(jmp_buf));
      return CSOUND_MEMORY;
    }
    newp = (MYFLT*)mrealloc(csound, (void*)(*p), sizeof(MYFLT) * newSize);
    memcpy((void*)&csound->exitjmp, (void*)&saved_exitjmp, sizeof(jmp_buf));
    i = (*oldSize);
    memset(&newp[i], '\0', (&newp[newSize-1]-&newp[i]));
    /* do { */
    /*   newp[i] = FL(0.0); */
    /* } while (++i < newSize); */
    (*p) = newp;
    (*oldSize) = newSize;
    return CSOUND_SUCCESS;
}

static CS_NOINLINE int chan_realloc_f(CSOUND *csound,
                                      void **p, int *oldSize,
                                      int newSize, void *init)
{
    volatile jmp_buf  saved_exitjmp;
    PVSDATEXT           *newp, *pp;
    int chans = newSize - *oldSize, i;
    PVSDAT   *fin = (PVSDAT *)init;

    memcpy((void*) &saved_exitjmp, (void*)&csound->exitjmp, sizeof(jmp_buf));
    if (setjmp(csound->exitjmp) != 0) {
      memcpy((void*)&csound->exitjmp, (void*)&saved_exitjmp, sizeof(jmp_buf));
      return CSOUND_MEMORY;
    }
    newp = (PVSDATEXT *)mrealloc(csound, *p,  sizeof(PVSDATEXT) * newSize);
    for (i=*oldSize; i < chans; i++) {
      pp             = &newp[i];
      pp->frame      = (float *)mmalloc(csound, (fin->N+2)*sizeof(float));
      pp->N          = fin->N;
      pp->overlap    = fin->overlap;
      pp->winsize    = fin->winsize;
      pp->wintype    = fin->wintype;
      pp->format     = fin->format;
      pp->framecount = fin->framecount;
    }
    memcpy((void*)&csound->exitjmp, (void*)&saved_exitjmp, sizeof(jmp_buf));
    (*p) = newp;
    (*oldSize) = newSize;

    return CSOUND_SUCCESS;
}

/**
* Sends a MYFLT value to the chani opcode (k-rate) at index 'n'.
* The bus is automatically extended if 'n' exceeds any previously used
* index value, clearing new locations to zero.
* Returns zero on success, CSOUND_ERROR if the index is invalid, and
* CSOUND_MEMORY if there is not enough memory to extend the bus.
*/
PUBLIC int csoundChanIKSet(CSOUND *csound, MYFLT value, int n)
{
    if (n < 0)
      return CSOUND_ERROR;
    if ((unsigned int)n >= (unsigned int)csound->nchanik) {
      int   err = chan_realloc(csound,
                               &(csound->chanik), &(csound->nchanik), n + 1);
      if (UNLIKELY(err))
        return err;
    }
    csound->chanik[n] = value;
    return CSOUND_SUCCESS;
}

/**
* Receives a MYFLT value from the chano opcode (k-rate) at index 'n'.
* The bus is automatically extended if 'n' exceeds any previously used
* index value, clearing new locations to zero.
* Returns zero on success, CSOUND_ERROR if the index is invalid, and
* CSOUND_MEMORY if there is not enough memory to extend the bus.
*/
PUBLIC int csoundChanOKGet(CSOUND *csound, MYFLT *value, int n)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    if ((unsigned int)n >= (unsigned int)csound->nchanok) {
      int   err = chan_realloc(csound,
                               &(csound->chanok), &(csound->nchanok), n + 1);
      if (UNLIKELY(err))
        return err;
    }
    (*value) = csound->chanok[n];
    return CSOUND_SUCCESS;
}

/**
* Sends ksmps MYFLT values to the chani opcode (a-rate) at index 'n'.
* The bus is automatically extended if 'n' exceeds any previously used
* index value, clearing new locations to zero.
* Returns zero on success, CSOUND_ERROR if the index is invalid, and
* CSOUND_MEMORY if there is not enough memory to extend the bus.
*/
PUBLIC int csoundChanIASet(CSOUND *csound, const MYFLT *value, int n)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    n *= csound->ksmps;
    if ((unsigned int)n >= (unsigned int)csound->nchania) {
      int   err = chan_realloc(csound, &(csound->chania),
                               &(csound->nchania), n + csound->ksmps);
      if (UNLIKELY(err))
        return err;
    }
    memcpy(&(csound->chania[n]), value, sizeof(MYFLT) * csound->ksmps);
    return CSOUND_SUCCESS;
}

/**
* Receives ksmps MYFLT values from the chano opcode (a-rate) at index 'n'.
* The bus is automatically extended if 'n' exceeds any previously used
* index value, clearing new locations to zero.
* Returns zero on success, CSOUND_ERROR if the index is invalid, and
* CSOUND_MEMORY if there is not enough memory to extend the bus.
*/
PUBLIC int csoundChanOAGet(CSOUND *csound, MYFLT *value, int n)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    n *= csound->ksmps;
    if ((unsigned int)n >= (unsigned int)csound->nchanoa) {
      int   err = chan_realloc(csound, &(csound->chanoa),
                               &(csound->nchanoa), n + csound->ksmps);
      if (UNLIKELY(err))
        return err;
    }
    memcpy(value, &(csound->chanoa[n]), sizeof(MYFLT) * csound->ksmps);
    return CSOUND_SUCCESS;
}

PUBLIC int csoundChanIKSetValue(CSOUND *csound, int n, MYFLT value)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    if ((unsigned int)n >= (unsigned int)csound->nchanik) {
      int   err = chan_realloc(csound,
                               &(csound->chanik), &(csound->nchanik), n + 1);
      if (UNLIKELY(err))
        return err;
    }
    csound->chanik[n] = value;
    return CSOUND_SUCCESS;
}

PUBLIC MYFLT csoundChanOKGetValue(CSOUND *csound, int n)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    if ((unsigned int)n >= (unsigned int)csound->nchanok) {
      int   err = chan_realloc(csound,
                               &(csound->chanok), &(csound->nchanok), n + 1);
      if (UNLIKELY(err))
        return err;
    }
    return csound->chanok[n];
}

PUBLIC int csoundChanIASetSample(CSOUND *csound, int n, int i, MYFLT sample)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    n *= csound->ksmps;
    if ((unsigned int)n >= (unsigned int)csound->nchanoa) {
      int   err = chan_realloc(csound, &(csound->chanoa),
                               &(csound->nchanoa), n + csound->ksmps);
      if (UNLIKELY(err))
        return err;
    }
    csound->chanoa[n + i] = sample;
    return CSOUND_SUCCESS;
}


PUBLIC MYFLT csoundChanOAGetSample(CSOUND *csound, int n, int i)
{
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    n *= csound->ksmps;
    if ((unsigned int)n >= (unsigned int)csound->nchanoa) {
      int   err = chan_realloc(csound, &(csound->chanoa),
                               &(csound->nchanoa), n + csound->ksmps);
      if (UNLIKELY(err))
        return err;
    }
    return csound->chanoa[n + i];
}


/**
* Sends a PVSDATEX fin to the pvsin opcode (f-rate) at index 'n'.
* The bus is automatically extended if 'n' exceeds any previously used
* index value, clearing new locations to zero.
* Returns zero on success, CSOUND_ERROR if the index is invalid or
* fsig framesizes are incompatible, and
* CSOUND_MEMORY if there is not enough memory to extend the bus.
*/
PUBLIC int csoundPvsinSet(CSOUND *csound, const PVSDATEXT *fin, int n)
{
    PVSDATEXT *fout = (PVSDATEXT *)csound->chanif;
    int size;
    if (UNLIKELY(n < 0))
      return CSOUND_ERROR;
    if ((unsigned int)n >= (unsigned int)csound->nchanif) {
      int   err = chan_realloc_f(csound, (void *)&(csound->chanif),
                                 &(csound->nchanif), n + 1,
                                 (void *)fin);
      if (UNLIKELY(err))
        return err;
      fout = (PVSDATEXT *)csound->chanif;
      memcpy(fout[n].frame, fin->frame, sizeof(float)*(fin->N+2));
      return CSOUND_SUCCESS;
    }
    size = fout[n].N < fin->N ? fout[n].N : fin->N;
    memcpy(&fout[n], fin, sizeof(PVSDATEXT)-sizeof(float *));
    if (LIKELY(size > 0))
       memcpy(fout[n].frame, fin->frame, sizeof(float)*(size+2));
    return CSOUND_SUCCESS;
}

/**
* Receives a PVSDATEX fout from the chano opcode (f-rate) at index 'n'.
* The bus is extended if n exceeds existing spaces, initialising
* it using the PVSDATEX fout struct parameters.
* Returns zero on success, CSOUND_ERROR if the index is invalid or
* if fsigs framesizes are incompatible
*/
PUBLIC int csoundPvsoutGet(CSOUND *csound, PVSDATEXT *fout, int n)
{
    PVSDATEXT *fin = (PVSDATEXT *)csound->chanof;
    int size;
    if (UNLIKELY(n < 0)) return CSOUND_ERROR;
    if (((unsigned int)n >= (unsigned int)csound->nchanof)) {
      int err = chan_realloc_f(csound, (void *)&(csound->chanof),
                               &(csound->nchanof), n + 1, (void *)fout);
      if (UNLIKELY(err))
        return err;
      fin = (PVSDATEXT *)csound->chanof;
      memset(fin[n].frame, 0, sizeof(float)*(fin[n].N+2));
      return CSOUND_SUCCESS;
    }
    size = fout->N < fin[n].N ? fout->N : fin[n].N;
    memcpy(fout, &fin[n], sizeof(PVSDATEXT)-sizeof(float *));
    if (LIKELY(size > 0))
      memcpy(fout->frame, fin[n].frame, sizeof(float)*(size+2));
    return CSOUND_SUCCESS;
}

/* ------------------------------------------------------------------------ */

#ifdef HAVE_C99
#  ifdef MYFLT2LRND
#    undef MYFLT2LRND
#  endif
#  ifndef USE_DOUBLE
#    define MYFLT2LRND  lrintf
#  else
#    define MYFLT2LRND  lrint
#  endif
#endif

int chani_opcode_perf_k(CSOUND *csound, ASSIGN *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, Str("chani: invalid index"));
    if ((unsigned int)n >= (unsigned int)csound->nchanik) {
      if (UNLIKELY(chan_realloc(csound, &(csound->chanik),
                                &(csound->nchanik), n + 1) != 0))
        return csound->PerfError(csound,
                                 Str("chani: memory allocation failure"));
    }
    *(p->r) = csound->chanik[n];
    return OK;
}

int chano_opcode_perf_k(CSOUND *csound, ASSIGN *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, Str("chano: invalid index"));
    if ((unsigned int)n >= (unsigned int)csound->nchanok) {
      if (UNLIKELY(chan_realloc(csound, &(csound->chanok),
                                &(csound->nchanok), n + 1) != 0))
        return csound->PerfError(csound,
                                 Str("chano: memory allocation failure"));
    }
    csound->chanok[n] = *(p->r);
    return OK;
}

int chani_opcode_perf_a(CSOUND *csound, ASSIGN *p)
{
    int     n = (int)MYFLT2LRND(*(p->a)) * csound->global_ksmps;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, Str("chani: invalid index"));
    if ((unsigned int)n >= (unsigned int)csound->nchania) {
      if (UNLIKELY(chan_realloc(csound, &(csound->chania),
                                &(csound->nchania), n + csound->global_ksmps) != 0))
        return csound->PerfError(csound,
                                 Str("chani: memory allocation failure"));
    }
    memcpy(p->r, &(csound->chania[n]), sizeof(MYFLT) * csound->ksmps);
    return OK;
}

int chano_opcode_perf_a(CSOUND *csound, ASSIGN *p)
{
    int     n = (int)MYFLT2LRND(*(p->a)) * csound->global_ksmps;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, Str("chano: invalid index"));
    if ((unsigned int)n >= (unsigned int)csound->nchanoa) {
      if (UNLIKELY(chan_realloc(csound, &(csound->chanoa),
                                &(csound->nchanoa), n + csound->global_ksmps) != 0))
        return csound->PerfError(csound,
                                 Str("chano: memory allocation failure"));
    }
    memcpy(&(csound->chanoa[n]), p->r, sizeof(MYFLT) * csound->ksmps);
    return OK;
}

int pvsin_init(CSOUND *csound, FCHAN *p)
{
    int N;
    N = p->init.N = (int32)(*p->N ? *p->N : 1024);
    p->init.overlap = (int32) (*p->overlap ? *p->overlap : p->init.N/4);
    p->init.winsize = (int32) (*p->winsize ? *p->winsize : p->init.N);
    p->init.wintype = (int32)(*p->wintype);
    p->init.format = (int32)(*p->format);
    p->init.framecount = 0;
    memcpy(p->r, &p->init, sizeof(PVSDAT)-sizeof(AUXCH));
    if (p->r->frame.auxp == NULL ||
        p->r->frame.size < sizeof(float) * (N + 2))
      csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->r->frame);
    return OK;
}

int pvsin_perf(CSOUND *csound, FCHAN *p)
{
    PVSDATEXT *fin = (PVSDATEXT *)csound->chanif;
    PVSDAT *fout = p->r;
    int     n = (int)MYFLT2LRND(*(p->a)), size;
    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, Str("pvsin: invalid index"));
    if (((unsigned int)n >= (unsigned int)csound->nchanif)){
      int err = chan_realloc_f(csound, (void *)&(csound->chanif),
                               &(csound->nchanif), n + 1,
                               (void *) &(p->init));
      if (UNLIKELY(err)) {
        return csound->PerfError(csound, Str("pvsin: memory allocation failure"));
      }
      else {
        fin = (PVSDATEXT *)csound->chanif;
        memset(fin[n].frame, 0, sizeof(float)*(fin[n].N+2));
      }
    }
    size = fin[n].N < fout->N ? fin[n].N : fout->N;
    memcpy(fout, &fin[n], sizeof(PVSDAT)-sizeof(AUXCH));
    memcpy(fout->frame.auxp, fin[n].frame, sizeof(float)*(size+2));
    return OK;
}

int pvsout_perf(CSOUND *csound, FCHAN *p)
{
    int     n = (int)MYFLT2LRND(*(p->a)), size;
    PVSDATEXT *fout = (PVSDATEXT *)csound->chanof;
    PVSDAT *fin = p->r;
    if (UNLIKELY(n < 0))
      return csound->PerfError(csound,Str("pvsout: invalid index"));

    if ((unsigned int)n >= (unsigned int)csound->nchanof) {
      if (UNLIKELY(chan_realloc_f(csound, (void *)&(csound->chanof),
                                  &(csound->nchanof), n + 1,
                                  (void *) fin) != 0))
        return csound->PerfError(csound,
                                 Str("pvsout: memory allocation failure"));
      else fout = (PVSDATEXT *)csound->chanof;
    }
    size = fout[n].N < fin->N ? fout[n].N : fin->N;
    memcpy(&fout[n], fin, sizeof(PVSDAT)-sizeof(AUXCH));
    memcpy(fout[n].frame, fin->frame.auxp, sizeof(float)*(size+2));
    return OK;
}
/* ======================================================================== */

/* "chn" opcodes and bus interface by Istvan Varga */

typedef struct controlChannelInfo_s {
    int     type;
    MYFLT   dflt;
    MYFLT   min;
    MYFLT   max;
} controlChannelInfo_t;

typedef struct channelEntry_s {
    struct channelEntry_s *nxt;
    controlChannelInfo_t  *info;
    MYFLT   *data;
    int     lock;               /* Multi-thread protection */
    int     type;
    char    name[1];
} channelEntry_t;

static int delete_channel_db(CSOUND *csound, void *p)
{
    channelEntry_t  **db, *pp;
    int             i;

    (void) p;
    db = (channelEntry_t**) csound->chn_db;
    if (db == NULL) {
      return 0;
    }
    for (i = 0; i < 256; i++) {
      while (db[i] != NULL) {
        pp = db[i];
        db[i] = pp->nxt;
        if (pp->info != NULL)
          free((void*) pp->info);
        free((void*) pp);
      }
    }
    csound->chn_db = NULL;
    free((void*) db);
    return 0;
}

static inline channelEntry_t *find_channel(CSOUND *csound, const char *name)
{
    if (csound->chn_db != NULL && name[0]) {
      channelEntry_t  *pp;
      pp = ((channelEntry_t**) csound->chn_db)[name_hash_2(csound, name)];
      for ( ; pp != NULL; pp = pp->nxt) {
        const char  *p1 = &(name[0]);
        const char  *p2 = &(pp->name[0]);
        while (1) {
          if (*p1 != *p2)
            break;
          if (*p1 == '\0')
            return pp;
          p1++, p2++;
        }
      }
    }
    return NULL;
}

static CS_NOINLINE channelEntry_t *alloc_channel(CSOUND *csound, MYFLT **p,
                                                 const char *name, int type)
{
    channelEntry_t  dummy;
    void            *pp;
    int             nbytes, nameOffs, dataOffs;

    (void) dummy;
    nameOffs = (int)((char*) &(dummy.name[0]) - (char*) &dummy);
    dataOffs = nameOffs + ((int)strlen(name) + 1);
    dataOffs += ((int)sizeof(MYFLT) - 1);
    dataOffs = (dataOffs / (int)sizeof(MYFLT)) * (int)sizeof(MYFLT);
    nbytes = dataOffs;
    if (*p == NULL) {
      switch (type & CSOUND_CHANNEL_TYPE_MASK) {
      case CSOUND_CONTROL_CHANNEL:
        nbytes += (int)sizeof(MYFLT);
        break;
      case CSOUND_AUDIO_CHANNEL:
        nbytes += ((int)sizeof(MYFLT) * csound->global_ksmps);
        break;
      case CSOUND_STRING_CHANNEL:
        nbytes += ((int)sizeof(MYFLT) * csound->strVarSamples);
        break;
      }
    }
    pp = (void*) malloc((size_t) nbytes);
    if (pp == NULL)
      return (channelEntry_t*) NULL;
    memset(pp, 0, (size_t) nbytes);
    if (*p == NULL)
      *p = (MYFLT*) ((char*) pp + (int)dataOffs);
    return (channelEntry_t*) pp;
}

static CS_NOINLINE int create_new_channel(CSOUND *csound, MYFLT **p,
                                          const char *name, int type)
{
    channelEntry_t  *pp;
    const char      *s;
    unsigned char   h;

    /* check for valid parameters and calculate hash value */
    if (UNLIKELY((type & (~51)) || !(type & 3) || !(type & 48)))
      return CSOUND_ERROR;
    s = name;
    if (UNLIKELY(!isalpha((unsigned char) *s)))
      return CSOUND_ERROR;
    h = (unsigned char) 0;
    do {
      h = strhash_tabl_8[(unsigned char) *(s++) ^ h];
    } while (isalnum((unsigned char) *s) ||
             *s == (char) '_' || *s == (char) '.');
    if (*s != (char) 0)
      return CSOUND_ERROR;
    /* create new empty database on first call */
    if (csound->chn_db == NULL) {
      if (UNLIKELY(csound->RegisterResetCallback(csound, NULL,
                                                 delete_channel_db) != 0))
        return CSOUND_MEMORY;
      csound->chn_db = (void*) calloc((size_t) 256, sizeof(channelEntry_t*));
      if (UNLIKELY(csound->chn_db == NULL))
        return CSOUND_MEMORY;
    }
    /* allocate new entry */
    pp = alloc_channel(csound, p, name, type);
    if (UNLIKELY(pp == NULL))
      return CSOUND_MEMORY;
    pp->nxt = ((channelEntry_t**) csound->chn_db)[h];
    pp->info = NULL;
    pp->data = (*p);
    pp->type = type;
    strcpy(&(pp->name[0]), name);
    ((channelEntry_t**) csound->chn_db)[h] = pp;

    return CSOUND_SUCCESS;
}

/**
* Stores a pointer to the specified channel of the bus in *p,
* creating the channel first if it does not exist yet.
* 'type' must be the bitwise OR of exactly one of the following values,
*   CSOUND_CONTROL_CHANNEL
*     control data (one MYFLT value)
*   CSOUND_AUDIO_CHANNEL
*     audio data (csoundGetKsmps(csound) MYFLT values)
*   CSOUND_STRING_CHANNEL
*     string data (MYFLT values with enough space to store
*     csoundGetStrVarMaxLen(csound) characters, including the
*     NULL character at the end of the string)
* and at least one of these:
*   CSOUND_INPUT_CHANNEL
*   CSOUND_OUTPUT_CHANNEL
* If the channel already exists, it must match the data type (control,
* audio, or string), however, the input/output bits are OR'd with the
* new value. Note that audio and string channels can only be created
* after calling csoundCompile(), because the storage size is not known
* until then.
* Return value is zero on success, or a negative error code,
*   CSOUND_MEMORY  there is not enough memory for allocating the channel
*   CSOUND_ERROR   the specified name or type is invalid
* or, if a channel with the same name but incompatible type already exists,
* the type of the existing channel. In the case of any non-zero return
* value, *p is set to NULL.
* Note: to find out the type of a channel without actually creating or
* changing it, set 'type' to zero, so that the return value will be either
* the type of the channel, or CSOUND_ERROR if it does not exist.
*/

PUBLIC int csoundGetChannelPtr(CSOUND *csound,
                               MYFLT **p, const char *name, int type)
{
    channelEntry_t  *pp;

    *p = (MYFLT*) NULL;
    if (UNLIKELY(name == NULL))
      return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (pp != NULL) {
      if ((pp->type ^ type) & CSOUND_CHANNEL_TYPE_MASK)
        return pp->type;
      pp->type |= (type & (CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL));
      *p = pp->data;
      return CSOUND_SUCCESS;
    }
    return create_new_channel(csound, p, name, type);
}

PUBLIC int *csoundGetChannelLock(CSOUND *csound,
                                const char *name, int type)
{
    channelEntry_t  *pp;

    if (UNLIKELY(name == NULL))
      return NULL;
    pp = find_channel(csound, name);
    return &pp->lock;
}

static int cmp_func(const void *p1, const void *p2)
{
    return strcmp(((CsoundChannelListEntry*) p1)->name,
                  ((CsoundChannelListEntry*) p2)->name);
}

/**
* Returns a list of allocated channels in *lst. A CsoundChannelListEntry
* structure contains the name and type of a channel, with the type having
* the same format as in the case of csoundGetChannelPtr().
* The return value is the number of channels, which may be zero if there
* are none, or CSOUND_MEMORY if there is not enough memory for allocating
* the list. In the case of no channels or an error, *lst is set to NULL.
* Notes: the caller is responsible for freeing the list returned in *lst
* with csoundDeleteChannelList(). The name pointers may become invalid
* after calling csoundReset().
*/

PUBLIC int csoundListChannels(CSOUND *csound, CsoundChannelListEntry **lst)
{
    channelEntry_t  *pp;
    size_t          i, n;

    *lst = (CsoundChannelListEntry*) NULL;
    if (csound->chn_db == NULL)
      return 0;
    /* count the number of channels */
    for (n = (size_t) 0, i = (size_t) 0; i < (size_t) 256; i++) {
      for (pp = ((channelEntry_t**) csound->chn_db)[i];
           pp != NULL;
           pp = pp->nxt, n++)
        ;
    }
    if (!n)
      return 0;
    /* create list, initially in unsorted order */
    *lst = (CsoundChannelListEntry*) malloc(n * sizeof(CsoundChannelListEntry));
    if (UNLIKELY(*lst == NULL))
      return CSOUND_MEMORY;
    for (n = (size_t) 0, i = (size_t) 0; i < (size_t) 256; i++) {
      for (pp = ((channelEntry_t**) csound->chn_db)[i];
           pp != NULL;
           pp = pp->nxt, n++) {
        (*lst)[n].name = pp->name;
        (*lst)[n].type = pp->type;
      }
    }
    /* sort list */
    qsort((void*) (*lst), n, sizeof(CsoundChannelListEntry), cmp_func);
    /* return the number of channels */
    return (int)n;
}

/**
* Releases a channel list previously returned by csoundListChannels().
*/

PUBLIC void csoundDeleteChannelList(CSOUND *csound, CsoundChannelListEntry *lst)
{
    (void) csound;
    if (lst != NULL) free(lst);
}

/**
* Sets special parameters for a control channel. The parameters are:
*   type:  must be one of CSOUND_CONTROL_CHANNEL_INT,
*          CSOUND_CONTROL_CHANNEL_LIN, or CSOUND_CONTROL_CHANNEL_EXP for
*          integer, linear, or exponential channel data, respectively,
*          or zero to delete any previously assigned parameter information
*   dflt:  the control value that is assumed to be the default, should be
*          greater than or equal to 'min', and less than or equal to 'max'
*   min:   the minimum value expected; if the control type is exponential,
*          it must be non-zero
*   max:   the maximum value expected, should be greater than 'min';
*          if the control type is exponential, it must be non-zero and
*          match the sign of 'min'
* Returns zero on success, or a non-zero error code on failure:
*   CSOUND_ERROR:  the channel does not exist, is not a control channel,
*                  or the specified parameters are invalid
*   CSOUND_MEMORY: could not allocate memory
*/

PUBLIC int csoundSetControlChannelParams(CSOUND *csound, const char *name,
                                         int type, MYFLT dflt,
                                         MYFLT min, MYFLT max)
{
    channelEntry_t  *pp;

    if (UNLIKELY(name == NULL))
      return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (UNLIKELY(pp == NULL))
      return CSOUND_ERROR;
    if (UNLIKELY((pp->type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL))
      return CSOUND_ERROR;
    if (!type) {
      if (pp->info != NULL) {
        free((void*) pp->info);
        pp->info = NULL;
      }
      return CSOUND_SUCCESS;
    }
    switch (type) {
    case CSOUND_CONTROL_CHANNEL_INT:
      dflt = (MYFLT) ((int32) MYFLT2LRND(dflt));
      min  = (MYFLT) ((int32) MYFLT2LRND(min));
      max  = (MYFLT) ((int32) MYFLT2LRND(max));
      break;
    case CSOUND_CONTROL_CHANNEL_LIN:
    case CSOUND_CONTROL_CHANNEL_EXP:
      break;
    default:
      return CSOUND_ERROR;
    }
    if (UNLIKELY(min >= max || dflt < min || dflt > max ||
                 (type == CSOUND_CONTROL_CHANNEL_EXP && ((min * max) <= FL(0.0)))))
      return CSOUND_ERROR;
    if (pp->info == NULL) {
      pp->info = (controlChannelInfo_t*) malloc(sizeof(controlChannelInfo_t));
      if (UNLIKELY(pp->info == NULL))
        return CSOUND_MEMORY;
    }
    pp->info->type = type;
    pp->info->dflt = dflt;
    pp->info->min  = min;
    pp->info->max  = max;
    return CSOUND_SUCCESS;
}

/**
* Returns special parameters (assuming there are any) of a control channel,
* previously set with csoundSetControlChannelParams().
* If the channel exists, is a control channel, and has the special parameters
* assigned, then the default, minimum, and maximum value is stored in *dflt,
* *min, and *max, respectively, and a positive value that is one of
* CSOUND_CONTROL_CHANNEL_INT, CSOUND_CONTROL_CHANNEL_LIN, and
* CSOUND_CONTROL_CHANNEL_EXP is returned.
* In any other case, *dflt, *min, and *max are not changed, and the return
* value is zero if the channel exists, is a control channel, but has no
* special parameters set; otherwise, a negative error code is returned.
*/

PUBLIC int csoundGetControlChannelParams(CSOUND *csound, const char *name,
                                         MYFLT *dflt, MYFLT *min, MYFLT *max)
{
    channelEntry_t  *pp;

    if (UNLIKELY(name == NULL))
      return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (pp == NULL)
      return CSOUND_ERROR;
    if ((pp->type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL)
      return CSOUND_ERROR;
    if (pp->info == NULL)
      return 0;
    (*dflt) = pp->info->dflt;
    (*min) = pp->info->min;
    (*max) = pp->info->max;
    return pp->info->type;
}

/**
* Sets callback function to be called by the opcodes 'chnsend' and
* 'chnrecv'. Should be called between csoundPreCompile() and
* csoundCompile().
* The callback function takes the following arguments:
*   CSOUND *csound
*     Csound instance pointer
*   const char *channelName
*     the channel name
*   MYFLT *channelValuePtr
*     pointer to the channel value. Control channels are a single MYFLT
*     value, while audio channels are an array of csoundGetKsmps(csound)
*     MYFLT values. In the case of string channels, the pointer should be
*     cast to char *, and points to a buffer of
*     csoundGetStrVarMaxLen(csound) bytes
*   int channelType
*     bitwise OR of the channel type (CSOUND_CONTROL_CHANNEL,
*     CSOUND_AUDIO_CHANNEL, or CSOUND_STRING_CHANNEL; use
*     channelType & CSOUND_CHANNEL_TYPE_MASK to extract the channel
*     type), and either CSOUND_INPUT_CHANNEL or CSOUND_OUTPUT_CHANNEL
*     to indicate the direction of the data transfer
* The callback is not preserved on csoundReset().
*/

PUBLIC void csoundSetChannelIOCallback(CSOUND *csound,
                                       CsoundChannelIOCallback_t func)
{
    csound->channelIOCallback_ = func;
}

/* ------------------------------------------------------------------------ */

/* perf time stub for printing "not initialised" error message */

int notinit_opcode_stub(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, Str("%s: not initialised"),
                             csound->GetOpcodeName(p));
}

/* print error message on failed channel query */

static CS_NOINLINE int print_chn_err(void *p, int err)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *msg;

    if (((OPDS*) p)->opadr != (SUBR) NULL)
      ((OPDS*) p)->opadr = (SUBR) notinit_opcode_stub;
    if (err == CSOUND_MEMORY)
      msg = "memory allocation failure";
    else if (err < 0)
      msg = "invalid channel name";
    else
      msg = "channel already exists with incompatible type";
    return csound->InitError(csound, Str(msg));
}

/* receive control value from bus at performance time */

static int chnget_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
    *(p->arg) = *(p->fp);
    return OK;
}

/* receive audio data from bus at performance time */

static int chnget_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    memcpy(p->arg, p->fp, sizeof(MYFLT)*csound->ksmps);
    return OK;
}

/* receive control value from bus at init time */

int chnget_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    *(p->arg) = *(p->fp);

    return OK;
}

/* init routine for chnget opcode (control data) */

int chnget_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->h.opadr = (SUBR) chnget_opcode_perf_k;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnget opcode (audio data) */

int chnget_opcode_init_a(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->h.opadr = (SUBR) chnget_opcode_perf_a;
      return OK;
    }
    return print_chn_err(p, err);
}

/* receive string value from bus at init time */

int chnget_opcode_init_S(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    strcpy((char*) p->arg, (char*) p->fp);

    return OK;
}

/* send control value to bus at performance time */

static int chnset_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
    *(p->fp) = *(p->arg);
    return OK;
}

/* send audio data to bus at performance time */

static int chnset_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    int *lock = p->lock;        /* Need lock for the channel */
    csoundSpinLock(lock);
    memcpy(p->fp, p->arg, sizeof(MYFLT)*csound->ksmps);
    csoundSpinUnLock(lock);
    return OK;
}

/* send audio data to bus at performance time, mixing to previous output */

static int chnmix_opcode_perf(CSOUND *csound, CHNGET *p)
{
    int   i = 0;
    int n = csound->ksmps;
    int *lock = p->lock;        /* Need lock for the channel */
    csoundSpinLock(lock);
    for (i=0; i<n; i++) {
      p->fp[i] += p->arg[i];
    }
    csoundSpinUnLock(lock);
    return OK;
}

/* clear an audio channel to zero at performance time */

static int chnclear_opcode_perf(CSOUND *csound, CHNCLEAR *p)
{
    int *lock = p->lock;        /* Need lock for the channel */
    csoundSpinLock(lock);
    memset(p->fp, 0, csound->ksmps*sizeof(MYFLT));
    csoundSpinUnLock(lock);
    return OK;
}

/* send control value to bus at init time */

int chnset_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;
    int *lock;        /* Need lock for the channel */

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    p->lock = lock = csoundGetChannelLock(csound, (char*) p->iname,
                                CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    csoundSpinLock(lock);
    *(p->fp) = *(p->arg);
    csoundSpinUnLock(lock);

    return OK;
}

/* init routine for chnset opcode (control data) */

int chnset_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname,
                                CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
      p->h.opadr = (SUBR) chnset_opcode_perf_k;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnset opcode (audio data) */

int chnset_opcode_init_a(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname,
                                CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
      p->h.opadr = (SUBR) chnset_opcode_perf_a;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnmix opcode */

int chnmix_opcode_init(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname,
                                CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
      p->h.opadr = (SUBR) chnmix_opcode_perf;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnclear opcode */

int chnclear_opcode_init(CSOUND *csound, CHNCLEAR *p)
{
    int   err;

    /* NOTE: p->imode is a pointer to the channel data here */
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
      p->h.opadr = (SUBR) chnclear_opcode_perf;
      return OK;
    }
    return print_chn_err(p, err);
}

/* send string to bus at init time */

int chnset_opcode_init_S(CSOUND *csound, CHNGET *p)
{
    int   err;
    int  *lock;
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname,
                              CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    if (UNLIKELY((int)strlen((char*) p->arg) >= csound->strVarMaxLen)) {
      /* can only happen with constants */
      return csound->InitError(csound, Str("string is too long"));
    }
    p->lock = lock =
      csoundGetChannelLock(csound, (char*) p->iname,
                           CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    csoundSpinLock(lock);
    strcpy((char*) p->fp, (char*) p->arg);
    csoundSpinUnLock(lock);

    return OK;
}

/* declare control channel, optionally with special parameters */

int chn_k_opcode_init(CSOUND *csound, CHN_OPCODE_K *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_CONTROL_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (err)
      return print_chn_err(p, err);
    type = (int)MYFLT2LRND(*(p->itype));
    err = csoundSetControlChannelParams(csound, (char*) p->iname, type,
                                        *(p->idflt), *(p->imin), *(p->imax));
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname, type);
      return OK;
    }
    if (err == CSOUND_MEMORY)
      return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));
}

/* declare audio channel */

int chn_a_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_AUDIO_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    return OK;
}

/* declare string channel */

int chn_S_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int   type, mode, err;

    mode = (int)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_STRING_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, type);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    p->lock = csoundGetChannelLock(csound, (char*) p->iname, type);
    return OK;
}

/* export new channel from global orchestra variable */

int chnexport_opcode_init(CSOUND *csound, CHNEXPORT_OPCODE *p)
{
    MYFLT       *dummy;
    const char  *argName;
    int         type = CSOUND_CONTROL_CHANNEL, mode, err;

    /* must have an output argument of type 'gi', 'gk', 'ga', or 'gS' */
    if (UNLIKELY(csound->GetOutputArgCnt(p) != 1))
      goto arg_err;
    argName = csound->GetOutputArgName(p, 0);
    if (UNLIKELY(argName == NULL))
      goto arg_err;
    if (UNLIKELY(argName[0] != 'g'))
      goto arg_err;
    switch ((int)argName[1]) {
    case 'i':
    case 'k':
      break;
    case 'a':
      type = CSOUND_AUDIO_CHANNEL;
      break;
    case 'S':
      type = CSOUND_STRING_CHANNEL;
      break;
    default:
      goto arg_err;
    }
    /* mode (input and/or output) */
    mode = (int)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
      return csound->InitError(csound, Str("invalid mode parameter"));
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;
    /* check if the channel already exists (it should not) */
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, 0);
    if (UNLIKELY(err >= 0))
      return csound->InitError(csound, Str("channel already exists"));
    /* now create new channel, using output variable for data storage */
    dummy = p->arg;
    /* THIS NEEDS A LOCK BUT DOES NOT EXIST YET */
    /* lock = csoundGetChannelLock(csound, (char*) p->iname, 0); */
    /* csoundSpinLock(lock); */
    err = create_new_channel(csound, &dummy, (char*) p->iname, type);
    /* csoundSpinLock(lock); */
    if (err)
      return print_chn_err(p, err);
    /* if control channel, set additional parameters */
    if ((type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL)
      return OK;
    type = (int)MYFLT2LRND(*(p->itype));
    err = csoundSetControlChannelParams(csound, (char*) p->iname, type,
                                        *(p->idflt), *(p->imin), *(p->imax));
    if (LIKELY(!err))
      return OK;
    if (err == CSOUND_MEMORY)
      return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));

 arg_err:
    return csound->InitError(csound, Str("invalid export variable"));
}

/* returns all parameters of a channel */

int chnparams_opcode_init(CSOUND *csound, CHNPARAMS_OPCODE *p)
{
    MYFLT *dummy;
    int   err;

    /* all values default to zero... */
    *(p->itype)    = FL(0.0);
    *(p->imode)    = FL(0.0);
    *(p->ictltype) = FL(0.0);
    *(p->idflt)    = FL(0.0);
    *(p->imin)     = FL(0.0);
    *(p->imax)     = FL(0.0);
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname, 0);
    /* ...if channel does not exist */
    if (err <= 0)
      return OK;
    /* type (control/audio/string) */
    *(p->itype) = (MYFLT) (err & 15);
    /* mode (input and/or output) */
    *(p->imode) = (MYFLT) ((err & 48) >> 4);
    /* check for control channel parameters */
    if ((err & 15) == CSOUND_CONTROL_CHANNEL) {
      err = csoundGetControlChannelParams(csound, (char*) p->iname,
                                          p->idflt, p->imin, p->imax);
      if (UNLIKELY(err > 0))
        *(p->ictltype) = (MYFLT) err;
    }
    return OK;
}

static int dummy_opcode_stub(CSOUND *csound, void *p)
{
    (void) csound;
    (void) p;
    return OK;
}

static int chn_send_recv_opcodes_perf(CSOUND *csound, CHNSEND *p)
{
    csound->channelIOCallback_(csound, p->name, p->fp, p->type);
    return OK;
}

static CS_NOINLINE int chn_send_recv_opcodes_init_(CSOUND *csound,
                                                   CHNSEND *p, int ioType)
{
    channelEntry_t  *pp;
    int             chnType, mode = 0;

    pp = find_channel(csound, (char*) p->iname);
    if (UNLIKELY(pp == (channelEntry_t*) NULL)) {
      p->h.opadr = (SUBR) notinit_opcode_stub;
      return csound->InitError(csound, Str("channel '%s' does not exist"),
                               (char*) p->iname);
    }
    if (UNLIKELY(!(pp->type & ioType))) {
      p->h.opadr = (SUBR) notinit_opcode_stub;
      return csound->InitError(csound, Str("channel '%s' is not an %s channel"),
                               Str(ioType == CSOUND_INPUT_CHANNEL ?
                                   "input" : "output"));
    }
    p->name = &(pp->name[0]);
    p->fp = pp->data;
    chnType = pp->type & CSOUND_CHANNEL_TYPE_MASK;
    p->type = chnType | ioType;
    if (chnType != CSOUND_AUDIO_CHANNEL) {
      if (*(p->imode) < FL(0.5))
        mode = (chnType == CSOUND_STRING_CHANNEL ? 1 : 3);
      else {
        mode = (int)(*(p->imode) + FL(0.5));
        if (UNLIKELY(mode > 3)) {
          p->h.opadr = (SUBR) notinit_opcode_stub;
          return csound->InitError(csound, Str("invalid mode parameter: %d"),
                                   mode);
        }
      }
    }
    else
      mode = 2;
    if (csound->channelIOCallback_ != (CsoundChannelIOCallback_t) NULL) {
      if (mode & 2)
        p->h.opadr = (SUBR) chn_send_recv_opcodes_perf;
      else
        p->h.opadr = (SUBR) dummy_opcode_stub;
      if (mode & 1)
        csound->channelIOCallback_(csound, p->name, p->fp, p->type);
    }
    else
      p->h.opadr = (SUBR) dummy_opcode_stub;

    return OK;
}

int chnrecv_opcode_init(CSOUND *csound, CHNSEND *p)
{
    return chn_send_recv_opcodes_init_(csound, p, CSOUND_INPUT_CHANNEL);
}

int chnsend_opcode_init(CSOUND *csound, CHNSEND *p)
{
    return chn_send_recv_opcodes_init_(csound, p, CSOUND_OUTPUT_CHANNEL);
}

/* ********************************************************************** */
/* *************** SENSING ********************************************** */
/* ********************************************************************** */

#if defined(__unix) || defined(__unix__) || defined(__MACH__)
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif
#  ifdef HAVE_TERMIOS_H
#    include <termios.h>
#  endif
#elif defined(WIN32)
#  include <conio.h>
#endif

int sensekey_perf(CSOUND *csound, KSENSE *p)
{
    int     keyCode = 0;
    int     retval;

    retval = csound->doCsoundCallback(csound, &keyCode,
                                      (p->keyDown != NULL ?
                                       CSOUND_CALLBACK_KBD_EVENT
                                       : CSOUND_CALLBACK_KBD_TEXT));
    if (retval > 0) {
      if (!p->evtbuf) {
#if defined(__unix) || defined(__unix__) || defined(__MACH__)
        if (csound->inChar_ < 0) {
#  if defined(WIN32)
          setvbuf(stdin, NULL, _IONBF, 0);  /* Does not seem to work */
#  elif defined(HAVE_TERMIOS_H)
          struct termios  tty;
          tcgetattr(0, &tty);
          tty.c_lflag &= (~ICANON);
          tcsetattr(0, TCSANOW, &tty);
#  endif
        }
#endif
        p->evtbuf = -1;
      }
      if (csound->inChar_ < 0) {
#if defined(__unix) || defined(__unix__) || defined(__MACH__)
        fd_set  rfds;
        struct timeval  tv;
        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        /* No waiting */
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval) {
          char    ch = (char) 0;
          read(0, &ch, 1);
          keyCode = (int)((unsigned char) ch);
          /* FD_ISSET(0, &rfds) will be true. */
        }
#else
        unsigned char ch = (unsigned char) 0;
#  ifdef WIN32
        if (_kbhit())
          ch = (unsigned char) _getch();
#  else
        ch = (unsigned char) getchar();
#  endif
        keyCode = (int)ch;
#endif
      }
      else if (csound->inChar_ > 0) {
        keyCode = csound->inChar_;
        csound->inChar_ = 0;
      }
      if (p->evtbuf != -1) {
        int     tmp = keyCode;
        keyCode = p->evtbuf;
        tmp = (keyCode < 0 ? tmp : (-1 - keyCode));
        p->evtbuf = (tmp != 0 ? tmp : -1);
      }
      else if (p->OUTOCOUNT>1 && p->keyDown != NULL)
        p->evtbuf = -1 - keyCode;
      if (keyCode < 0)
        keyCode = 65535 - keyCode;
    }
    else if (retval < 0) {
      keyCode = 0;
    }
    *(p->ans) = (MYFLT) ((keyCode & (int)0xFFFF) ?
                         (keyCode & (int)0xFFFF) : -1);
    if (p->OUTOCOUNT>1 && p->keyDown != NULL)
      *(p->keyDown) = (MYFLT) ((keyCode > 0 && keyCode < 65536) ? 1 : 0);

    return OK;
}

