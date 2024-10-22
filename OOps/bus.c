/*  bus.c:

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
         Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
         02110-1301 USA
*/

/*                      BUS.C           */
#include "csoundCore.h"
#include "arrays.h"
#include <setjmp.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#if defined(__wasi__)
#include <sys/select.h>
#endif

#include "bus.h"
#include "namedins.h"

/* For sensing opcodes */
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

#ifdef USE_DOUBLE
#  define MYFLT_INT_TYPE int64_t
#else
#  define MYFLT_INT_TYPE int32_t
#endif

int32_t chani_opcode_perf_k(CSOUND *csound, CHNVAL *p)
{
    int32_t     n = (int32_t)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int32_t   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
        return csound->PerfError(csound, &(p->h),Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, (void **) &val, chan_name,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("chani error %d:"
                                     "channel not found or not right type"), err);
    *(p->r) = *val;
    return OK;
}

int32_t chano_opcode_perf_k(CSOUND *csound, CHNVAL *p)
{
    int32_t     n = (int32_t)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int32_t   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
        return csound->PerfError(csound,&(p->h),Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, (void **) &val, chan_name,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("chano error %d:"
                                     "channel not found or not right type"), err);
    *val = *(p->r);
    return OK;
}

int32_t chani_opcode_perf_a(CSOUND *csound, CHNVAL *p)
{
    int32_t     n = (int32_t)MYFLT2LRND(*(p->a));
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    char chan_name[16];
    int32_t   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
        return csound->PerfError(csound, &(p->h),Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, (void **) &val, chan_name,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("chani error %d:"
                                     "channel not found or not right type"), err);
    if (UNLIKELY(offset)) memset(p->r, '\0', offset * sizeof(MYFLT));
    memcpy(&p->r[offset], &val[offset],
           sizeof(MYFLT) * (CS_KSMPS-offset-early));
    if (UNLIKELY(early))
        memset(&p->r[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    return OK;
}

int32_t chano_opcode_perf_a(CSOUND *csound, CHNVAL *p)
{
    int32_t     n = (int32_t)MYFLT2LRND(*(p->a));
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    char chan_name[16];
    int32_t   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
        return csound->PerfError(csound, &(p->h),Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, (void **) &val, chan_name,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("chano error %d:"
                                     "channel not found or not right type"), err);

    if (UNLIKELY(offset)) memset(&val, '\0', offset * sizeof(MYFLT));
    memcpy(&val[offset], &p->r[offset],
           sizeof(MYFLT) * (CS_KSMPS-offset-early));

    if (UNLIKELY(early))
        memset(&val[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    return OK;
}

int32_t pvsin_init(CSOUND *csound, FCHAN *p)
{
    int32_t N;
    if(GetTypeForArg(p->a) == &CS_VAR_TYPE_S) 
      strncpy(p->name,((STRINGDAT *)p->a)->data, MAX_CHAN_NAME);
    else {
      p->n = (int32_t) MYFLT2LRND(*p->a);
      snprintf(p->name, MAX_CHAN_NAME+1, "%i", p->n);
    }
    
    if (csoundGetChannelPtr(csound, (void **) &(p->f), p->name,
                            CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL)
        == CSOUND_SUCCESS){
        p->lock = (spin_lock_t *)
                csoundGetChannelLock(csound, p->name);
        csoundSpinLock(p->lock);
        memcpy(&(p->init), p->f, sizeof(PVSDAT)-sizeof(AUXCH));
        csoundSpinUnLock(p->lock);
    } 

    N = p->init.N = (int32_t)(*p->N ? *p->N : p->init.N);
    p->init.overlap = (int32_t) (*p->overlap ? *p->overlap : p->init.overlap);
    p->init.winsize = (int32_t) (*p->winsize ? *p->winsize : p->init.winsize);
    p->init.wintype = (int32_t)(*p->wintype);
    p->init.format = (int32_t)(*p->format);
    p->init.framecount = 0;
    memcpy(p->r, &p->init, sizeof(PVSDAT)-sizeof(AUXCH));
    if (p->r->frame.auxp == NULL ||
        p->r->frame.size < sizeof(float) * (N + 2))
        csound->AuxAlloc(csound, (N + 2) * sizeof(float), &p->r->frame);
    return OK;
}

int32_t pvsin_perf(CSOUND *csound, FCHAN *p)
{
    PVSDAT *fout = p->r;
    int32_t   err, size, flag = 0;
 
    if(GetTypeForArg(p->a) == &CS_VAR_TYPE_S) {
      if(strcmp(((STRINGDAT *)p->a)->data, p->name)) {
        strncpy(p->name, ((STRINGDAT *)p->a)->data, MAX_CHAN_NAME);
        flag = 1;
      }
    }
    else {
      if(p->n != (int32_t) MYFLT2LRND(*p->a)) {
       p->n = (int32_t) MYFLT2LRND(*p->a);
       snprintf(p->name, MAX_CHAN_NAME+1, "%i", p->n);
       flag = 1;
      }
    }
    if(flag) {   
      err = csoundGetChannelPtr(csound, (void **) &(p->f), p->name,
                              CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("pvsin error %d:"
                                     "channel not found or not right type"), err);
      p->lock = (spin_lock_t *) csoundGetChannelLock(csound,p->name);
    }
    size = p->f->N < fout->N ? p->f->N : fout->N;
    csoundSpinLock(p->lock);
    memcpy(fout, p->f, sizeof(PVSDAT)-sizeof(AUXCH));
    if(p->f->frame.auxp != NULL)
        memcpy(fout->frame.auxp, p->f->frame.auxp, sizeof(float)*(size+2));
    else memset(fout->frame.auxp, 0, sizeof(float)*(size+2));
    csoundSpinUnLock(p->lock);
    return OK;
}

int32_t pvsout_init(CSOUND *csound, FCHAN *p)
{
    PVSDAT *fin = p->r;
    if(GetTypeForArg(p->a) == &CS_VAR_TYPE_S) 
      strncpy(p->name, ((STRINGDAT *)p->a)->data, MAX_CHAN_NAME);
    else {
      p->n = (int32_t) MYFLT2LRND(*p->a);
      snprintf(p->name, MAX_CHAN_NAME+1, "%i", p->n);
    }
    
    if (csoundGetChannelPtr(csound, (void **) &(p->f), p->name,
                            CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL)
        == CSOUND_SUCCESS){
        p->lock = (spin_lock_t *)
                csoundGetChannelLock(csound, p->name);
        csoundSpinLock(p->lock);
        if(p->f->frame.auxp == NULL || p->f->N < fin->N) {
            csound->AuxAlloc(csound, (p->f->N + 2) * sizeof(float), &p->f->frame);
        } 
        memcpy(p->f, fin, sizeof(PVSDAT)-sizeof(AUXCH));
        csoundSpinUnLock(p->lock);
    }
    return OK;
}


int32_t pvsout_perf(CSOUND *csound, FCHAN *p)
{
    PVSDAT *fin = p->r;
    int32_t   err, size, flag = 0;
    if(GetTypeForArg(p->a) == &CS_VAR_TYPE_S) {
      if(strcmp(((STRINGDAT *)p->a)->data, p->name)) {
        strncpy(p->name, ((STRINGDAT *)p->a)->data,  MAX_CHAN_NAME);
        flag = 1;
      }
    }
    else {
      if(p->n != (int32_t) MYFLT2LRND(*p->a)) {
       p->n = (int32_t) MYFLT2LRND(*p->a);
       snprintf(p->name, MAX_CHAN_NAME+1, "%i", p->n);
       flag = 1;
      }
    }
    if(flag) {
      err = csoundGetChannelPtr(csound, (void **) &(p->f), p->name,
                              CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
        return csound->PerfError(csound, &(p->h),
                                 Str("pvsout error %d:"
                                     "channel not found or not right type"), err);
    p->lock = (spin_lock_t *) csoundGetChannelLock(csound, p->name);
    }
    csoundSpinLock(p->lock);
    size = fin->N < p->f->N ? fin->N : p->f->N;
    memcpy(p->f, fin, sizeof(PVSDAT)-sizeof(AUXCH));
    if(p->f->frame.auxp != NULL)
        memcpy(p->f->frame.auxp, fin->frame.auxp, sizeof(float)*(size+2));
    csoundSpinUnLock(p->lock);
    return OK;
}

PUBLIC int32_t csoundPvsDataFFTSize(const PVSDAT *pvsdat) {
  return pvsdat->N;
}

PUBLIC int32_t csoundPvsDataOverlap(const PVSDAT *pvsdat) {
  return pvsdat->overlap;
}

PUBLIC int32_t csoundPvsDataWindowSize(const PVSDAT *pvsdat) {
  return pvsdat->winsize;
}

PUBLIC int32_t csoundPvsDataFormat(const PVSDAT *pvsdat) {
  return pvsdat->format;
}

PUBLIC uint32_t csoundPvsDataFramecount(const PVSDAT *pvsdat) {
  return pvsdat->framecount;
}

PUBLIC const float *csoundGetPvsData(const PVSDAT *pvsdat) {
  return (const float *) pvsdat->frame.auxp; 
}

PUBLIC void csoundSetPvsData(PVSDAT *pvsdat, const float *frame) {
  memcpy(pvsdat->frame.auxp, frame, pvsdat->frame.size);
}

PUBLIC PVSDAT *csoundInitPvsChannel(CSOUND *csound, const char* name,
                                    int32_t size, int32_t overlap, int32_t winsize,
                                 int32_t wintype, int32_t format) {
    PVSDAT *p;
    if (csoundGetChannelPtr(csound, (void **) &p, name,
                            CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL
                            | CSOUND_OUTPUT_CHANNEL)
        == CSOUND_SUCCESS){
      if(p->frame.auxp == NULL) {
      p->N = (int32_t) size;
      p->overlap = (int32_t) overlap;
      p->winsize = (int32_t) winsize;
      p->wintype = (int32_t) wintype;
      p->format = (int32_t) format;
      p->framecount = 0;
      csound->AuxAlloc(csound, (p->N + 2) * sizeof(float), &p->frame);
      }
      return (PVSDAT *) p;
    } else return NULL;
}

/* ======================================================================== */
/* "chn" opcodes and bus interface by Istvan Varga */

static int32_t delete_channel_db(CSOUND *csound, void *p)
{
    CONS_CELL *head, *values;
    IGN(p);
    if (csound->chn_db == NULL) {
        return 0;
    }

    head = values = cs_hash_table_values(csound, csound->chn_db);

    if (head != NULL) {
        while(values != NULL) {
            CHNENTRY* entry = values->value;

            if ((entry->type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL) {
                csound->Free(csound, entry->hints.attributes);
            }
            /* SY - 2014.07.14 - Don't free entry->data and rely on Csound memory db
               to free it; fixes issue with RTTI and chnexport of a global var, which
               maps to the CS_VAR_MEM's memblock, which is not what is allocated; other
               vars will be freed since they were Calloc'd */
            /*          csound->Free(csound, entry->data); */
            entry->datasize = 0;
            values = values->next;
        }
        cs_cons_free(csound, head);
    }

    cs_hash_table_mfree_complete(csound, csound->chn_db);
    csound->chn_db = NULL;
    return 0;
}

static inline CHNENTRY *find_channel(CSOUND *csound, const char *name)
{
    if (csound->chn_db != NULL && name[0]) {
        return (CHNENTRY*) cs_hash_table_get(csound, csound->chn_db, (char*) name);
    }
    return NULL;
}

void set_channel_data_ptr(CSOUND *csound,
                          const char *name, void *ptr, int32_t newSize)
{
    find_channel(csound, name)->data = (MYFLT *) ptr;
    find_channel(csound, name)->datasize = newSize;
}

#define INIT_STRING_CHANNEL_DATASIZE 256

static CS_NOINLINE CHNENTRY *alloc_channel(CSOUND *csound,
                                           const char *name, int32_t type)
{
    CHNENTRY      *pp;
    int32_t           dsize = 0;
    const CS_TYPE *varType;
    switch (type & CSOUND_CHANNEL_TYPE_MASK) {
        case CSOUND_CONTROL_CHANNEL:
            dsize = sizeof(MYFLT);
            varType = &CS_VAR_TYPE_K;
            break;
        case CSOUND_AUDIO_CHANNEL:
            dsize = ((int32_t)sizeof(MYFLT) * csound->ksmps);
            varType = &CS_VAR_TYPE_A;
            break;
        case CSOUND_STRING_CHANNEL:
            dsize = sizeof(STRINGDAT);
            varType = &CS_VAR_TYPE_S;
            break;
        case CSOUND_PVS_CHANNEL:
            dsize = sizeof(PVSDAT);
            varType = &CS_VAR_TYPE_F;
            break;
        case CSOUND_ARRAY_CHANNEL:
            dsize = sizeof(ARRAYDAT);
            varType = &CS_VAR_TYPE_ARRAY;
            break;
    }
    
    pp = (CHNENTRY *) csound->Calloc(csound,
                                     (size_t) sizeof(CHNENTRY) + strlen(name) + 1);
    if (pp == NULL) return (CHNENTRY*) NULL;
    pp->data = (MYFLT *) csound->Calloc(csound, dsize); 

    if ((type & CSOUND_CHANNEL_TYPE_MASK) == CSOUND_STRING_CHANNEL) {
        ((STRINGDAT*) pp->data)->size = 128;
        ((STRINGDAT*) pp->data)->data = csound->Calloc(csound, 128 * sizeof(char));
    }

    csoundSpinLockInit(&pp->lock);
    pp->datasize = dsize;
    pp->varType = varType;

    return (CHNENTRY*) pp;
}

static CS_NOINLINE int32_t create_new_channel(CSOUND *csound, const char *name,
                                              int32_t type)
{
    CHNENTRY      *pp;
    /* check for valid parameters and calculate hash value */
    if (UNLIKELY(!(type & 48)))
        return CSOUND_ERROR;

    /* create new empty database if not allocated */
    if (csound->chn_db == NULL) {
        csound->chn_db = cs_hash_table_create(csound);
        if (UNLIKELY(csound->RegisterResetCallback(csound, NULL,
                                                   delete_channel_db) != 0))
            return CSOUND_MEMORY;
        if (UNLIKELY(csound->chn_db == NULL))
            return CSOUND_MEMORY;
    }
    /* allocate new entry */
    pp = alloc_channel(csound, name, type);
    if (UNLIKELY(pp == NULL))
        return CSOUND_MEMORY;
    pp->hints.behav = 0;
    pp->type = type;
    strcpy(&(pp->name[0]), name);

    cs_hash_table_put(csound, csound->chn_db, (char*)name, pp);

    return CSOUND_SUCCESS;
}

PUBLIC const char *csoundGetChannelVarTypeName(CSOUND *csound, const char *name) {
  CHNENTRY *pp = find_channel(csound, name);
  if(pp)
    return pp->varType->varTypeName;
  else return NULL;
}

PUBLIC const CS_TYPE *csoundGetChannelVarType(CSOUND *csound, const char *name) {
  CHNENTRY *pp = find_channel(csound, name);
  if(pp)
    return pp->varType;
  else return NULL;
}


PUBLIC int32_t csoundGetChannelPtr(CSOUND *csound, void **p,
                               const char *name, int32_t type)
{
    CHNENTRY  *pp;

    *p = (MYFLT*) NULL;
    if (UNLIKELY(name == NULL))
        return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (!pp) {
        if (create_new_channel(csound, name, type) == CSOUND_SUCCESS) {
            pp = find_channel(csound, name);
        }
    }
    if (pp != NULL) {
        if ((pp->type ^ type) & CSOUND_CHANNEL_TYPE_MASK)
            return pp->type;
        pp->type |= (type & (CSOUND_INPUT_CHANNEL | CSOUND_OUTPUT_CHANNEL));
        *p = pp->data;
        return CSOUND_SUCCESS;
    }
    return CSOUND_ERROR;
}

PUBLIC int32_t csoundGetChannelDatasize(CSOUND *csound, const char *name){

    CHNENTRY  *pp;
    pp = find_channel(csound, name);
    if (pp == NULL) return 0;
    else {
        /* the reason for this is that if chnexport is
           used with strings, the datasize might become
           invalid */
        if ((pp->type & CSOUND_STRING_CHANNEL) == CSOUND_STRING_CHANNEL)
          return (int32_t) ((STRINGDAT*)pp->data)->size;
        // NEED TO CHECK FOR ARRAYS
        if((pp->type & CSOUND_ARRAY_CHANNEL) == CSOUND_ARRAY_CHANNEL)
            return (int32_t) ((ARRAYDAT*)pp->data)->allocated;
        return pp->datasize;
    }
}


int32_t *csoundGetChannelLock(CSOUND *csound, const char *name)
{
    CHNENTRY  *pp;

    if (UNLIKELY(name == NULL))
        return NULL;
    pp = find_channel(csound, name);
    if (pp) {
        return (int32_t*) &pp->lock;
    }
    else return NULL;
}

static int32_t cmp_func(const void *p1, const void *p2)
{
    return strcmp(((controlChannelInfo_t*) p1)->name,
                  ((controlChannelInfo_t*) p2)->name);
}

PUBLIC int32_t csoundListChannels(CSOUND *csound, controlChannelInfo_t **lst)
{
    CHNENTRY  *pp;
    size_t     n;
    CONS_CELL* channels;

    *lst = (controlChannelInfo_t*) NULL;
    if (csound->chn_db == NULL)
        return 0;

    channels = cs_hash_table_values(csound, csound->chn_db);
    n = cs_cons_length(channels);

    if (!n)
        return 0;

    /* create list, initially in unsorted order */
    //  csound->Malloc and the caller has to free it.
    // if not, it will be freed on reset
    *lst = (controlChannelInfo_t*) csound->Malloc(csound,
                                                  n * sizeof(controlChannelInfo_t));
    if (UNLIKELY(*lst == NULL))
        return CSOUND_MEMORY;

    n = 0;
    while (channels != NULL) {
        pp = channels->value;
        (*lst)[n].name = pp->name;
        (*lst)[n].type = pp->type;
        (*lst)[n].hints = pp->hints;
        channels = channels->next;
        n++;
    }

    /* sort list */
    qsort((void*) (*lst), n, sizeof(controlChannelInfo_t), cmp_func);
    /* return the number of channels */
    return (int32_t)n;
}

PUBLIC void csoundDeleteChannelList(CSOUND *csound, controlChannelInfo_t *lst)
{
    //(void) csound;
    if (lst != NULL) csound->Free(csound, lst);
}

PUBLIC int32_t csoundSetControlChannelHints(CSOUND *csound, const char *name,
                                            controlChannelHints_t hints)
{
    CHNENTRY  *pp;

    if (UNLIKELY(name == NULL))
        return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (UNLIKELY(pp == NULL))
        return CSOUND_ERROR;
    if (UNLIKELY((pp->type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL))
        return CSOUND_ERROR;
    if  (hints.behav == CSOUND_CONTROL_CHANNEL_NO_HINTS) {
        pp->hints.behav = CSOUND_CONTROL_CHANNEL_NO_HINTS;
        return 0;
    }
    if  (hints.behav == CSOUND_CONTROL_CHANNEL_INT) {
        hints.dflt = (MYFLT) ((int32_t) MYFLT2LRND(hints.dflt));
        hints.min  = (MYFLT) ((int32_t) MYFLT2LRND(hints.min));
        hints.max  = (MYFLT) ((int32_t) MYFLT2LRND(hints.max));
    }
    if (UNLIKELY(hints.min > hints.max || hints.dflt < hints.min ||
                 hints.dflt > hints.max ||
                 (hints.behav == CSOUND_CONTROL_CHANNEL_EXP &&
                  ((hints.min * hints.max) <= FL(0.0))))) {
        return CSOUND_ERROR;
    }

    pp->hints = hints;
    if (hints.attributes) {
        pp->hints.attributes
                = (char *) csound->Malloc(csound,
                                          (strlen(hints.attributes) + 1)* sizeof(char));
        strcpy(pp->hints.attributes, hints.attributes);
    }
    return CSOUND_SUCCESS;
}

/**
* Returns special parameters (assuming there are any) of a control channel,
* previously set with csoundSetControlChannelHints().
* If the channel exists, is a control channel, and has the special parameters
* assigned, then the default, minimum, and maximum value is stored in *dflt,
* *min, and *max, respectively, and a positive value that is one of
* CSOUND_CONTROL_CHANNEL_INT, CSOUND_CONTROL_CHANNEL_LIN, and
* CSOUND_CONTROL_CHANNEL_EXP is returned.
* In any other case, *dflt, *min, and *max are not changed, and the return
* value is zero if the channel exists, is a control channel, but has no
* special parameters set; otherwise, a negative error code is returned.
*/

PUBLIC int32_t csoundGetControlChannelHints(CSOUND *csound, const char *name,
                                            controlChannelHints_t *hints)
{
    CHNENTRY  *pp;

    if (UNLIKELY(name == NULL))
        return CSOUND_ERROR;
    pp = find_channel(csound, name);
    if (pp == NULL)
        return CSOUND_ERROR;
    if ((pp->type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL)
        return CSOUND_ERROR;
    if (pp->hints.behav == 0)
        return CSOUND_ERROR;
    *hints = pp->hints;
    if (pp->hints.attributes) {
        hints->attributes
                = (char *) csound->Malloc(csound,
                                          strlen(pp->hints.attributes) + 1);
        strcpy(hints->attributes, pp->hints.attributes);
    }
    return 0;
}

/* ------------------------------------------------------------------------ */

/* perf time stub for printing "not initialised" error message */

int32_t notinit_opcode_stub(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, &(((CHNGET *)p)->h),
                             Str("%s: not initialised"),
                             GetOpcodeName(p));
}

/* print error message on failed channel query */

static CS_NOINLINE void print_chn_err_perf(void *p, int32_t err)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *msg;

    if (((OPDS*) p)->perf != (SUBR) NULL)
        ((OPDS*) p)->perf = (SUBR) notinit_opcode_stub;
    if (err == CSOUND_MEMORY)
        msg = "memory allocation failure";
    else if (err < 0)
        msg = "invalid channel name";
    else
        msg = "channel already exists with incompatible type";
    csound->Warning(csound, "%s", Str(msg));
}

static CS_NOINLINE int32_t print_chn_err(void *p, int32_t err)
{
    CSOUND      *csound = ((OPDS*) p)->insdshead->csound;
    const char  *msg;

    if (((OPDS*) p)->perf != (SUBR) NULL)
        ((OPDS*) p)->perf = (SUBR) notinit_opcode_stub;
    if (err == CSOUND_MEMORY)
        msg = "memory allocation failure";
    else if (err < 0)
        msg = "invalid channel name";
    else
        msg = "channel already exists with incompatible type";
    return csound->InitError(csound, "%s", Str(msg));
}


/* receive control value from bus at performance time */
static int32_t chnget_opcode_perf_k(CSOUND* csound, CHNGET* p)
{
    if (strncmp(p->chname, p->iname->data, MAX_CHAN_NAME)
        || !strcmp(p->iname->data, ""))
    {
        int32_t err = csoundGetChannelPtr(csound, (void **)&(p->fp),
                                          (char*) p->iname->data,
                                          CSOUND_CONTROL_CHANNEL |
                                          CSOUND_INPUT_CHANNEL);
        if (err==0){
            p->lock = (spin_lock_t*)
              csoundGetChannelLock(csound, (char*) p->iname->data);
            strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
        }
        else {
            print_chn_err_perf(p, err);
            return OK;
        }
    }

#if defined(MSVC)
    volatile union {
    MYFLT d;
    MYFLT_INT_TYPE i;
    } x;
    x.i = InterlockedExchangeAdd64((MYFLT_INT_TYPE *) p->fp, 0);
    *(p->arg) = x.d;
#elif defined(HAVE_ATOMIC_BUILTIN)
    volatile union {
        MYFLT d;
        MYFLT_INT_TYPE i;
    } x;
    x.i = __atomic_load_n((MYFLT_INT_TYPE*) p->fp, __ATOMIC_SEQ_CST);
    *(p->arg) = x.d;
#else
    *(p->arg) = *(p->fp);
#endif
    return OK;
}

/* receive audio data from bus at performance time */
static int32_t chnget_opcode_perf_a(CSOUND* csound, CHNGET* p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early = p->h.insdshead->ksmps_no_end;

    if (strncmp(p->chname, p->iname->data, MAX_CHAN_NAME)
        || !strcmp(p->iname->data, ""))
    {
        int32_t err = csoundGetChannelPtr(csound, (void **)&(p->fp),
                                          (char*) p->iname->data,
                                          CSOUND_AUDIO_CHANNEL |
                                          CSOUND_INPUT_CHANNEL);
        if (err==0){
            p->lock = (spin_lock_t*)
              csoundGetChannelLock(csound, (char*) p->iname->data);
            strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
        }
        else {
            print_chn_err_perf(p, err);
            return OK;
        }
    } 
 
    if (CS_KSMPS ==(uint32_t) csound->ksmps){
        csoundSpinLock(p->lock);
        if (UNLIKELY(offset)) memset(p->arg, '\0', offset);
        memcpy(&p->arg[offset], p->fp, sizeof(MYFLT)*(CS_KSMPS-offset-early));
        if (UNLIKELY(early))
            memset(&p->arg[CS_KSMPS-early], '\0', sizeof(MYFLT)*early);
        csoundSpinUnLock(p->lock);
    }
    else {
        csoundSpinLock(p->lock);
        if (UNLIKELY(offset)) memset(p->arg, '\0', offset);
        memcpy(&p->arg[offset], &(p->fp[offset+p->pos]),
               sizeof(MYFLT)*(CS_KSMPS-offset-early));
        if (UNLIKELY(early))
            memset(&p->arg[CS_KSMPS-early], '\0', sizeof(MYFLT)*early);
        p->pos += CS_KSMPS;
        p->pos %= (csound->ksmps-offset);
        csoundSpinUnLock(p->lock);
    }

    return OK;
}

/* receive control value from bus at init time */
int32_t chnget_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int32_t   err;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
        return print_chn_err(p, err);
#if defined(MSVC)
    {
    union {
        MYFLT d;
        MYFLT_INT_TYPE i;
    } x;
    x.i = InterlockedExchangeAdd64((MYFLT_INT_TYPE *)p->fp, 0);
    *(p->arg) = x.d;
    }
#elif defined(HAVE_ATOMIC_BUILTIN)
    {
        union {
            MYFLT d;
            MYFLT_INT_TYPE i;
        } x;
        x.i = __atomic_load_n((MYFLT_INT_TYPE *) p->fp, __ATOMIC_SEQ_CST);
        *(p->arg) =  x.d;
    }
#else
    *(p->arg) = *(p->fp);
#endif

    return OK;
}

/* init routine for i-rate chnget array opcode (control data) */
int32_t chnget_array_opcode_init_i(CSOUND* csound, CHNGETARRAY* p)
{
    int32_t index = 0;
    ARRAYDAT* arr = (ARRAYDAT*) p->iname;

    p->arraySize = arr->sizes[0];
    p->channels = (STRINGDAT*) arr->data;

    tabinit(csound, p->arrayDat, p->arraySize, &(p->h));

    int32_t err;
    MYFLT* fp;

    for (index = 0; index<p->arraySize; index++)
    {
      err = csoundGetChannelPtr(csound, (void **)&fp,
                                  p->channels[index].data,
                                  CSOUND_CONTROL_CHANNEL |
                                  CSOUND_INPUT_CHANNEL);

        if (UNLIKELY(err))
            return print_chn_err(p, err);
#if defined(MSVC)
        {
        union {
            MYFLT d;
            MYFLT_INT_TYPE i;
        } x;
        x.i = InterlockedExchangeAdd64((MYFLT_INT_TYPE *)fp, 0);
        p->arrayDat->data[index] = x.d;
        }
#elif defined(HAVE_ATOMIC_BUILTIN)
        {
            union {
                MYFLT d;
                MYFLT_INT_TYPE i;
            } x;
            x.i = __atomic_load_n((MYFLT_INT_TYPE*) fp, __ATOMIC_SEQ_CST);
            p->arrayDat->data[index] = x.d;
        }
#else
        p->arrayDat->data[index] = *(fp);
#endif
    }
    return OK;
}

/* init routine for k, a and S chnget array opcodes */
int32_t chnget_array_opcode_init(CSOUND* csound, CHNGETARRAY* p)
{
    ARRAYDAT* arr = (ARRAYDAT*) p->iname;
    int32_t index = 0;
    p->arraySize = arr->sizes[0];
    p->channels = (STRINGDAT*) arr->data;
    p->channelPtrs = (MYFLT **) csound->Malloc(csound, p->arraySize*sizeof(MYFLT*)); // VL: surely an array of pointers?
    tabinit(csound, p->arrayDat, p->arraySize,  &(p->h));

    int32_t err;
    int32_t channelType;

    if (strcmp("k", p->arrayDat->arrayType->varTypeName) == 0)
        channelType = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;
    else if(strcmp("a", p->arrayDat->arrayType->varTypeName) == 0)
        channelType = CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL;
    else
        channelType = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    STRINGDAT *strings = (STRINGDAT *)p->arrayDat->data;
    for (index = 0; index<p->arraySize; index++)
    {
        if(strcmp(p->channels[index].data, "")) {
            err = csoundGetChannelPtr(csound, (void **) &p->channelPtrs[index],
                                      p->channels[index].data,
                                      channelType);

            if (LIKELY(!err)) {
                p->lock = (spin_lock_t *)
                  csoundGetChannelLock(csound,p->channels[index].data);
                strNcpy(p->chname, p->channels[index].data, MAX_CHAN_NAME);

                if(channelType == (CSOUND_STRING_CHANNEL |
                                   CSOUND_INPUT_CHANNEL)){
                    csoundSpinLock(p->lock);
                    strings[index].data =
                      cs_strdup(csound,
                                ((STRINGDAT *)p->channelPtrs[index])->data);
                    strings[index].size =
                      strlen(((STRINGDAT *)p->channelPtrs[index])->data + 1);
                    csoundSpinUnLock(p->lock);
                }
            }
        }
        else{
            return csound->InitError(csound, "%s%s", Str("invalid channel name:"),
                    !strcmp(p->channels[index].data, "") ?
                                     Str("\"empty\"") :
                                     Str(p->channels[index].data));
        }
    }

    if(channelType == (CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL))
        p->h.perf = (SUBR) chnget_array_opcode_perf_k;
    else if(channelType == (CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL))
        p->h.perf = (SUBR) chnget_array_opcode_perf_a;
    else
        p->h.perf = (SUBR) chnget_array_opcode_perf_S;

    return OK;
}

/* perf routine for chnget array opcode (control data) */
int32_t chnget_array_opcode_perf_k(CSOUND* csound, CHNGETARRAY* p)
{
    int32_t index = 0;

    for (index = 0; index<p->arraySize; index++)
    {
#if defined(MSVC)
        volatile union {
        MYFLT d;
        MYFLT_INT_TYPE i;
        } x;
        x.i = InterlockedExchangeAdd64((MYFLT_INT_TYPE *)
                                       p->channelPtrs[index], 0);
        p->arrayDat->data[index] = x.d;
#elif defined(HAVE_ATOMIC_BUILTIN)
        volatile union {
            MYFLT d;
            MYFLT_INT_TYPE i;
        } x;
        x.i = __atomic_load_n((MYFLT_INT_TYPE*) p->channelPtrs[index],
                              __ATOMIC_SEQ_CST);
        p->arrayDat->data[index] = x.d;
#else
        p->arrayDat->data[index] = *(p->channelPtrs[index]);
#endif

    }
    return OK;
}

/* perf routine for chnget array opcode (audio data) */
int32_t chnget_array_opcode_perf_a(CSOUND *csound, CHNGETARRAY *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    int32_t index = 0;
    int32_t blockIndex = 0;
    MYFLT* outPtr;
    for (index = 0; index<p->arraySize; index++) {
        blockIndex = csound->ksmps*index;

        if (CS_KSMPS == (uint32_t) csound->ksmps) {
            csoundSpinLock(p->lock);
            if (UNLIKELY(offset))
              memset(&p->arrayDat->data[blockIndex], '\0', offset);
            memcpy(&p->arrayDat->data[blockIndex+offset],
                   p->channelPtrs[index],
                   sizeof(MYFLT) * (CS_KSMPS - offset - early));
            if (UNLIKELY(early))
                memset(&p->arrayDat->data[blockIndex+CS_KSMPS - early],
                       '\0', sizeof(MYFLT) * early);
            csoundSpinUnLock(p->lock);
        } else {
            csoundSpinLock(p->lock);
            if (UNLIKELY(offset)) memset(&outPtr, '\0', offset);
            memcpy(&p->arrayDat->data[blockIndex+offset],
                   &(p->channelPtrs[index][offset + p->pos]),
                   sizeof(MYFLT) * (CS_KSMPS - offset - early));
            if (UNLIKELY(early))
                memset(&p->arrayDat->data[blockIndex+CS_KSMPS - early],
                       '\0', sizeof(MYFLT) * early);
            p->pos += CS_KSMPS;
            p->pos %= (csound->ksmps - offset);
            csoundSpinUnLock(p->lock);
        }
    }

    return OK;
}


int32_t chnget_array_opcode_perf_S(CSOUND* csound, CHNGETARRAY* p)
{
    STRINGDAT *strings = (STRINGDAT *)p->arrayDat->data;
    int32_t err;
    int32_t index = 0;

    for (index = 0; index<p->arraySize; index++) {
        err = csoundGetChannelPtr(csound, (void **) &p->channelPtrs[index],
                                  p->channels[index].data,
                                  CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);

        if (UNLIKELY(err))
            return print_chn_err(p, err);

        p->lock = (spin_lock_t *) csoundGetChannelLock(csound,
                                                       (char *)
                                                       p->channels[index].data);
        csoundSpinLock(p->lock);
        strings[index].data = cs_strdup(csound, ((STRINGDAT *)
                                                 p->channelPtrs[index])->data);
        strings[index].size = strlen(((STRINGDAT *)
                                      p->channelPtrs[index])->data + 1);
        csoundSpinUnLock(p->lock);
    }

    return OK;
}

/* chnset array opcode init function for S arrays */

int32_t chnset_array_opcode_init_i(CSOUND *csound, CHNGETARRAY *p)
{
    int32_t   err;

    int32_t index = 0;
    ARRAYDAT* valueArr = (ARRAYDAT*) p->arrayDat;
    ARRAYDAT* channelArr = (ARRAYDAT*) p->iname;
    p->arraySize = channelArr->sizes[0];
    p->channels = (STRINGDAT*) channelArr->data;
    p->channelPtrs = (MYFLT **) csound->Malloc(csound,
                                               p->arraySize*sizeof(MYFLT*)); 
    for (index = 0; index<p->arraySize; index++) {
        err = csoundGetChannelPtr(csound, (void **) &p->channelPtrs[index],
                                  (char *) p->channels[index].data,
                                  CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
        if (UNLIKELY(err))
            return print_chn_err(p, err);

#if defined(MSVC)
        volatile union {
      MYFLT d;
      MYFLT_INT_TYPE i;
    } x;
    x.d = valueArr->data[index];
    InterlockedExchange64((MYFLT_INT_TYPE *) p->channelPtrs[index], x.i);
#elif defined(HAVE_ATOMIC_BUILTIN)
        union {
            MYFLT d;
            MYFLT_INT_TYPE i;
        } x;
        x.d = valueArr->data[index];
        __atomic_store_n((MYFLT_INT_TYPE *)(p->channelPtrs[index]),
                         x.i, __ATOMIC_SEQ_CST);
#else
     {
      spin_lock_t *lock;       /* Need lock for the channel */
      p->lock = lock =  (spin_lock_t *)
        csoundGetChannelLock(csound, (char*) p->iname->data);
      csoundSpinLock(lock);
      *(p->channelPtrs[index]) =  valueArr->data[index];
      csoundSpinUnLock(lock);
    }
#endif
    }
    return OK;
}

/* init routine for chnset array opcodes - a, k and S */

int32_t chnset_array_opcode_init(CSOUND* csound, CHNGETARRAY* p)
{
    int32_t err;
    int32_t index = 0;

    ARRAYDAT* channelArr = (ARRAYDAT*) p->iname;
    p->arraySize = channelArr->sizes[0];
    p->channels = (STRINGDAT*) channelArr->data;
    p->channelPtrs = csound->Malloc(csound, p->arraySize*sizeof(MYFLT*)); 

    int32_t channelType;
    if (strcmp("k", p->arrayDat->arrayType->varTypeName) == 0)
        channelType = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;
    else if(strcmp("a", p->arrayDat->arrayType->varTypeName) == 0)
        channelType = CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL;
    else
        channelType = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    for (index = 0; index<p->arraySize; index++) {
        err = csoundGetChannelPtr(csound, (void **) &(p->channelPtrs[index]),
                                  (char *) p->channels[index].data,
                                  channelType);
        if (LIKELY(!err)) {
            p->lock = (spin_lock_t *)
              csoundGetChannelLock(csound, (char *) p->channels[index].data);
            strNcpy(p->chname, p->channels[index].data, MAX_CHAN_NAME);
        }
    }

    if(channelType == (CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL))
        p->h.perf = (SUBR) chnset_array_opcode_perf_k;
    else if(channelType == (CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL))
        p->h.perf = (SUBR) chnset_array_opcode_perf_a;
    else
        p->h.perf = (SUBR) chnset_array_opcode_perf_S;

    return OK;
}

/* send control value to bus at performance time */

int32_t chnset_array_opcode_perf_k(CSOUND *csound, CHNGETARRAY *p)
{
    int32_t index = 0;
    ARRAYDAT* valueArr = (ARRAYDAT*) p->arrayDat;

    for (index = 0; index<p->arraySize; index++) {
        if (strncmp(p->chname, p->channels[index].data, MAX_CHAN_NAME)) {
            int32_t err = csoundGetChannelPtr(csound, (void **) &(p->channelPtrs[index]),
                                              (char *) p->channels[index].data,
                                              CSOUND_CONTROL_CHANNEL |
                                              CSOUND_INPUT_CHANNEL);
            if (err == 0) {
                p->lock = (spin_lock_t *)
                  csoundGetChannelLock(csound, (char *)
                                       p->channels[index].data);
            } else
                print_chn_err_perf(p, err);
        }

#if defined(MSVC)
        volatile union {
          MYFLT d;
          MYFLT_INT_TYPE i;
        } x;
        x.d = valueArr->data[index];
        InterlockedExchange64((MYFLT_INT_TYPE *) p->channelPtrs[index], x.i);
#elif defined(HAVE_ATOMIC_BUILTIN)
        union {
            MYFLT d;
            MYFLT_INT_TYPE i;
        } x;
        x.d = valueArr->data[index];
        __atomic_store_n((MYFLT_INT_TYPE *) (p->channelPtrs[index]),
                         x.i, __ATOMIC_SEQ_CST);
#else
        csoundSpinLock(p->lock);
        *(p->channelPtrs[index]) = valueArr->data[index];
        csoundSpinUnLock(p->lock);
#endif
    }
    return OK;
}

/* perf routine for chnset array opcode (audio data) */

int32_t chnset_array_opcode_perf_a(CSOUND *csound, CHNGETARRAY *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    ARRAYDAT* valueArr = (ARRAYDAT*) p->arrayDat;
    int32_t index = 0;
    int32_t blockIndex = 0;

    for (index = 0; index<p->arraySize; index++) {
        if(CS_KSMPS == (uint32_t) csound->ksmps){
            blockIndex = csound->ksmps*index;
            /* Need lock for the channel */
            csoundSpinLock(p->lock);
            if (UNLIKELY(offset)) memset(p->channelPtrs[index],
                                         '\0', sizeof(MYFLT)*offset);
            memcpy(&p->channelPtrs[index][offset], &valueArr->data[blockIndex+offset],
                   sizeof(MYFLT)*(CS_KSMPS-offset-early));
            if (UNLIKELY(early))
                memset(&p->channelPtrs[index][early],
                       '\0', sizeof(MYFLT)*(CS_KSMPS-early));
            csoundSpinUnLock(p->lock);
        } else {
            /* Need lock for the channel */
            csoundSpinLock(p->lock);
            if (UNLIKELY(offset)) memset(p->channelPtrs[index],
                                         '\0', sizeof(MYFLT)*offset);
            memcpy(&p->channelPtrs[index][offset+p->pos],
                   &valueArr->data[blockIndex+offset],
                   sizeof(MYFLT)*(CS_KSMPS-offset-early));
            if (UNLIKELY(early))
                memset(&p->channelPtrs[index][early],
                       '\0', sizeof(MYFLT)*(CS_KSMPS-early));
            p->pos += CS_KSMPS;
            p->pos %= (csound->ksmps-offset);
            csoundSpinUnLock(p->lock);
        }
    }

    return OK;
}

int32_t chnset_array_opcode_perf_S(CSOUND* csound, CHNGETARRAY* p)
{
    int32_t err;
    int32_t index = 0;
    STRINGDAT *strings = (STRINGDAT *)p->arrayDat->data;

    for (index = 0; index<p->arraySize; index++)
    {
        if(strcmp(p->channels[index].data, "")) {
            err = csoundGetChannelPtr(csound, (void **) &p->channelPtrs[index],
                                      (char *) p->channels[index].data,
                                      CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
            if (UNLIKELY(err))
                return print_chn_err(p, err);
            p->lock = (spin_lock_t *)
              csoundGetChannelLock(csound, (char *) p->channels[index].data);
            csoundSpinLock(p->lock);
            ((STRINGDAT *)p->channelPtrs[index])->data =
              cs_strdup(csound, strings[index].data);
            ((STRINGDAT *)p->channelPtrs[index])->size =
              strlen(strings[index].data + 1);
            csoundSpinUnLock(p->lock);
        }
    }

    return OK;
}

/* init routine for chnget opcode (control data) */
int32_t chnget_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int32_t   err;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (LIKELY(!err)) {
        p->lock =   (spin_lock_t *)
          csoundGetChannelLock(csound, (char*) p->iname->data);
        strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
    }

    p->h.perf = (SUBR) chnget_opcode_perf_k;
    return OK;
}

/* init routine for chnget opcode (audio data) */
int32_t chnget_opcode_init_a(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    p->pos = 0;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (LIKELY(!err))
    {
        p->lock = (spin_lock_t*)
          csoundGetChannelLock(csound, (char*) p->iname->data);
        strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
    }

    p->h.perf = (SUBR) chnget_opcode_perf_a;
    return OK;
}

/* receive string value from bus at init time */
int32_t chnget_opcode_init_S(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    char* s = ((STRINGDAT*) p->arg)->data;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
    p->lock = (spin_lock_t*) csoundGetChannelLock(csound, (char*) p->iname->data);

    if (LIKELY(!err))
    {
        csoundSpinLock(p->lock);
        if (((STRINGDAT*) p->fp)->data!=NULL)
        {
            if (((STRINGDAT*) p->fp)->size>((STRINGDAT*) p->arg)->size){
                if (s!=NULL) csound->Free(csound, s);
                s = cs_strdup(csound, ((STRINGDAT*) p->fp)->data);
                ((STRINGDAT*) p->arg)->data = s;
                ((STRINGDAT*) p->arg)->size = strlen(s)+1;
            }
            else strcpy(((STRINGDAT*) p->arg)->data, ((STRINGDAT*) p->fp)->data);
        }
        csoundSpinUnLock(p->lock);
    }
    return OK;
}


int32_t chnget_opcode_perf_S(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    char* s = ((STRINGDAT*) p->arg)->data;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
    p->lock = (spin_lock_t*) csoundGetChannelLock(csound, (char*) p->iname->data);

    if (UNLIKELY(err))
        return print_chn_err(p, err);

    if (s!=NULL && ((STRINGDAT*) p->fp)->data!=NULL &&
        strcmp(s, ((STRINGDAT*) p->fp)->data)==0)
        return OK;

    csoundSpinLock(p->lock);

    if (((STRINGDAT*) p->fp)->data!=NULL)
    {
        if (((STRINGDAT*) p->arg)->size<=((STRINGDAT*) p->fp)->size)
        {
            if (s!=NULL) csound->Free(csound, s);
            s = cs_strdup(csound, ((STRINGDAT*) p->fp)->data);
            ((STRINGDAT*) p->arg)->data = s;
            ((STRINGDAT*) p->arg)->size = strlen(s)+1;
        }
        else strcpy(((STRINGDAT*) p->arg)->data, ((STRINGDAT*) p->fp)->data);
    }
    csoundSpinUnLock(p->lock);
    return OK;
}


/* send control value to bus at performance time */

static int32_t chnset_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
    if(strncmp(p->chname, p->iname->data, MAX_CHAN_NAME)){
        int32_t err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                                          CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
        if(err == 0) {
            p->lock = (spin_lock_t *)
              csoundGetChannelLock(csound, (char*) p->iname->data);
            strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
        }
        else
            print_chn_err_perf(p, err);
    } // else return csound->PerfError(csound, &p->h, "invalid channel name");

#if defined(MSVC)
    volatile union {
      MYFLT d;
      MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    InterlockedExchange64((MYFLT_INT_TYPE *) p->fp, x.i);
#elif defined(HAVE_ATOMIC_BUILTIN)
    union {
        MYFLT d;
        MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    __atomic_store_n((MYFLT_INT_TYPE *)(p->fp),x.i, __ATOMIC_SEQ_CST);
#else
    csoundSpinLock(p->lock);
    *(p->fp) = *(p->arg);
    csoundSpinUnLock(p->lock);
#endif
    return OK;
}

/* send audio data to bus at performance time */
static int32_t chnset_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(CS_KSMPS == (uint32_t) csound->ksmps){
        /* Need lock for the channel */
        csoundSpinLock(p->lock);
        if (UNLIKELY(offset)) memset(p->fp, '\0', sizeof(MYFLT)*offset);
        memcpy(&p->fp[offset], &p->arg[offset],
               sizeof(MYFLT)*(CS_KSMPS-offset-early));
        if (UNLIKELY(early))
            memset(&p->fp[early], '\0', sizeof(MYFLT)*(CS_KSMPS-early));
        csoundSpinUnLock(p->lock);
    } else {
        /* Need lock for the channel */
        csoundSpinLock(p->lock);
        if (UNLIKELY(offset)) memset(p->fp, '\0', sizeof(MYFLT)*offset);
        memcpy(&p->fp[offset+p->pos], &p->arg[offset],
               sizeof(MYFLT)*(CS_KSMPS-offset-early));
        if (UNLIKELY(early))
            memset(&p->fp[early], '\0', sizeof(MYFLT)*(CS_KSMPS-early));
        p->pos += CS_KSMPS;
        p->pos %= (csound->ksmps-offset);
        csoundSpinUnLock(p->lock);
    }
    return OK;
}

/* send audio data to bus at performance time, mixing to previous output */

static int32_t chnmix_opcode_perf(CSOUND *csound, CHNGET *p)
{
    uint32_t n = 0;
    IGN(csound);
    uint32_t nsmps = CS_KSMPS;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if (UNLIKELY(early)) nsmps -= early;
    /* Need lock for the channel */
    csoundSpinLock(p->lock);
    for (n=offset; n<nsmps; n++) {
        p->fp[n] += p->arg[n];
    }
    csoundSpinUnLock(p->lock);
    return OK;
}

/* clear an audio channel to zero at performance time */

static int32_t chnclear_opcode_perf(CSOUND *csound, CHNCLEAR *p)
{
    int32_t i, n=p->INCOUNT;
    /* Need lock for the channel */
    IGN(csound);
    for (i=0; i<n; i++) {
        csoundSpinLock(p->lock[i]);
        memset(p->fp[i], 0, CS_KSMPS*sizeof(MYFLT)); /* Should this leave start? */
        csoundSpinUnLock(p->lock[i]);
    }
    return OK;
}

/* send control value to bus at init time */
int32_t chnset_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int32_t   err;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
        return print_chn_err(p, err);
#if defined(MSVC)
    volatile union {
      MYFLT d;
      MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    InterlockedExchange64((MYFLT_INT_TYPE *) p->fp, x.i);
#elif defined(HAVE_ATOMIC_BUILTIN)
    union {
        MYFLT d;
        MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    __atomic_store_n((MYFLT_INT_TYPE *)(p->fp),x.i, __ATOMIC_SEQ_CST);
#else
    {
      spin_lock_t *lock;       /* Need lock for the channel */
      p->lock = lock =  (spin_lock_t *)
        csoundGetChannelLock(csound, (char*) p->iname->data);
      csoundSpinLock(lock);
      *(p->fp) = *(p->arg);
      csoundSpinUnLock(lock);
    }
#endif
    return OK;
}

/* init routine for chnset opcode (control data) */
int32_t chnset_opcode_init_k(CSOUND* csound, CHNGET* p)
{
    int32_t err;

    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
        p->lock = (spin_lock_t*) csoundGetChannelLock(csound, (char*) p->iname->data);
    } else return print_chn_err(p, err);

    p->h.perf = (SUBR) chnset_opcode_perf_k;
    return OK;
}

/* init routine for chnset opcode (audio data) */
int32_t chnset_opcode_init_a(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    p->pos = 0;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
        p->lock = (spin_lock_t*) csoundGetChannelLock(csound, (char*) p->iname->data);
    } else return print_chn_err(p, err);

    p->h.perf = (SUBR) chnset_opcode_perf_a;
    return OK;
}

/* init routine for chnmix opcode */

int32_t chnmix_opcode_init(CSOUND *csound, CHNGET *p)
{
    int32_t   err;

    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
        p->lock = (spin_lock_t *)csoundGetChannelLock(csound, (char*) p->iname->data);
        p->h.perf = (SUBR) chnmix_opcode_perf;
        return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnclear opcode */
int32_t chnclear_opcode_init(CSOUND *csound, CHNCLEAR *p)
{
    int32_t   err;
    int32_t   i, n = (int32_t)p->INCOUNT;
    for (i=0; i<n; i++) {
        /* NOTE: p->imode is a pointer to the channel data here */
      err = csoundGetChannelPtr(csound, (void **) &(p->fp[i]),
                                (char*) p->iname[i]->data,
                                  CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
        if (LIKELY(!err)) {
            p->lock[i] = (spin_lock_t *)
              csoundGetChannelLock(csound,(char*) p->iname[i]->data);
        }
        else return print_chn_err(p, err);
    }
    p->h.perf = (SUBR) chnclear_opcode_perf;
    return OK;
}

/* send string to bus at init time */

int32_t chnset_opcode_init_S(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    spin_lock_t* lock;
    char* s = ((STRINGDAT*) p->arg)->data;

    err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    // size = csoundGetChannelDatasize(csound, p->iname->data);
    if (LIKELY(!err))
    {

        if (s==NULL) return NOTOK;
        p->lock = lock = (spin_lock_t*)
                csoundGetChannelLock(csound, (char*) p->iname->data);
        csoundSpinLock(lock);
        if (strlen(s)>=(uint32_t) ((STRINGDAT*) p->fp)->size)
        {
            if (((STRINGDAT*) p->fp)->data!=NULL)
                csound->Free(csound, ((STRINGDAT*) p->fp)->data);
            ((STRINGDAT*) p->fp)->data = cs_strdup(csound, s);
            ((STRINGDAT*) p->fp)->size = strlen(s)+1;

            //set_channel_data_ptr(csound, p->iname->data,p->fp, strlen(s)+1);
        }
        else if (((STRINGDAT*) p->fp)->data!=NULL)
            strcpy(((STRINGDAT*) p->fp)->data, s);
        csoundSpinUnLock(lock);
    } else return print_chn_err(p, err);

    return OK;
}

int32_t chnset_opcode_perf_S(CSOUND* csound, CHNGET* p)
{
    int32_t err;
    spin_lock_t* lock;
    char* s = ((STRINGDAT*) p->arg)->data;

    if ((err = csoundGetChannelPtr(csound, (void **)&(p->fp), (char*) p->iname->data,
                                   CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)))
        return err;
    // size = csoundGetChannelDatasize(csound, p->iname->data);

    if (s==NULL) return NOTOK;
    if (((STRINGDAT*) p->fp)->data
        && strcmp(s, ((STRINGDAT*) p->fp)->data)==0)
        return OK;

    p->lock = lock = (spin_lock_t*)
            csoundGetChannelLock(csound, (char*) p->iname->data);
    csoundSpinLock(lock);
    if (strlen(s)>=(uint32_t) ((STRINGDAT*) p->fp)->size)
    {
        if (((STRINGDAT*) p->fp)->data!=NULL)
            csound->Free(csound, ((STRINGDAT*) p->fp)->data);
        ((STRINGDAT*) p->fp)->data = cs_strdup(csound, s);
        ((STRINGDAT*) p->fp)->size = strlen(s)+1;
        //set_channel_data_ptr(csound, p->iname->data,p->fp, strlen(s)+1);
        //printf("p: %s: %s \n", p->iname->data, ((STRINGDAT *)p->fp)->data);
    }
    else if (((STRINGDAT*) p->fp)->data!=NULL)
        strcpy(((STRINGDAT*) p->fp)->data, s);
    csoundSpinUnLock(lock);
    //printf("%s \n", (char *)p->fp);
    return OK;
}

/* declare control channel, optionally with special parameters */

int32_t chn_k_opcode_init_(CSOUND *csound, CHN_OPCODE_K *p, int32_t mode)
{
    MYFLT *dummy;
    int32_t   type, err;
    controlChannelHints_t hints;
    hints.attributes = NULL;
    hints.max = hints.min = hints.dflt = FL(0.0);
    hints.x = hints.y = hints.height = hints.width = 0;

    // mode = (int32_t)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
        return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_CONTROL_CHANNEL;
    if (mode & 1)
        type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
        type |= CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &dummy, (char*) p->iname->data, type);
    if (err)
        return print_chn_err(p, err);
    hints.behav = CSOUND_CONTROL_CHANNEL_NO_HINTS;
    if ((int)MYFLT2LRND(*(p->itype)) == 1)
        hints.behav = CSOUND_CONTROL_CHANNEL_INT;
    else if ((int32_t)MYFLT2LRND(*(p->itype)) == 2)
        hints.behav |= CSOUND_CONTROL_CHANNEL_LIN;
    else if ((int32_t)MYFLT2LRND(*(p->itype)) == 3)
        hints.behav |= CSOUND_CONTROL_CHANNEL_EXP;
    if ((int32_t)MYFLT2LRND(*(p->itype)) != 0) {
        hints.attributes = 0;
        if (p->INOCOUNT > 10) {
            hints.attributes = p->Sattributes->data;
        }
        hints.dflt = *(p->idflt);
        hints.min = *(p->imin);
        hints.max = *(p->imax);
        hints.x = *(p->ix);
        hints.y = *(p->iy);
        hints.width = *(p->iwidth);
        hints.height = *(p->iheight);
    }
    err = csoundSetControlChannelHints(csound, (char*) p->iname->data, hints);
    if (LIKELY(!err)) {
        p->lock = (spin_lock_t *)csoundGetChannelLock(csound, (char*) p->iname->data);
        return OK;
    }
    if (err == CSOUND_MEMORY)
        return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));
}

int32_t chn_k_opcode_init(CSOUND *csound, CHN_OPCODE_K *p)
{
    int32_t mode = (int32_t)MYFLT2LRND(*(p->imode));
    return chn_k_opcode_init_(csound, p, mode);
}

int32_t chn_k_opcode_init_S(CSOUND *csound, CHN_OPCODE_K *p)
{
    STRINGDAT *smode = (STRINGDAT *)p->imode;
    int32_t mode;
    if(!strcmp("rw", smode->data))
        mode = 3;
    else if(!strcmp("r", smode->data))
        mode = 1;
    else if(!strcmp("w", smode->data))
        mode = 2;
    else
        return csound->InitError(csound, Str("invalid mode, should be r, w, rw"));
    return chn_k_opcode_init_(csound, p, mode);
}


/* declare audio channel */
int32_t chn_a_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int32_t   type, mode, err;

    mode = (int32_t)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
        return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_AUDIO_CHANNEL;
    if (mode & 1)
        type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
        type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, (void **) &dummy, (char*) p->iname->data, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);
    return OK;
}

/* declare string channel */
int32_t chn_S_opcode_init(CSOUND *csound, CHN_OPCODE *p)
{
    MYFLT *dummy;
    int32_t   type, mode, err;

    mode = (int32_t)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
        return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_STRING_CHANNEL;
    if (mode & 1)
        type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
        type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, (void **) &dummy, (char*) p->iname->data, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);
    p->lock = (spin_lock_t *) csoundGetChannelLock(csound, (char*) p->iname->data);
    return OK;
}




/* export new channel from global orchestra variable */
int32_t chnexport_opcode_init(CSOUND *csound, CHNEXPORT_OPCODE *p)
{
    MYFLT       *dummy;
    const char  *argName;
    int32_t         type = CSOUND_CONTROL_CHANNEL, mode, err;
    controlChannelHints_t hints;
    CHNENTRY *chn;

    /* must have an output argument of type 'gi', 'gk', 'ga', or 'gS' */
    if (UNLIKELY(GetOutputArgCnt((OPDS *) p) != 1))
        goto arg_err;
    argName = GetOutputArgName((OPDS *) p, 0);
    if (UNLIKELY(argName == NULL))
        goto arg_err;
    if (UNLIKELY(argName[0] != 'g'))
        goto arg_err;
    switch ((int32_t)argName[1]) {
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
    mode = (int32_t)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
        return csound->InitError(csound, Str("invalid mode parameter"));
    if (mode & 1)
        type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
        type |= CSOUND_OUTPUT_CHANNEL;
    /* check if the channel already exists (it should not) */
    err = csoundGetChannelPtr(csound, (void **) &dummy, (char*) p->iname->data, 0);
    if (UNLIKELY(err >= 0))
        return csound->InitError(csound, Str("channel already exists"));
    /* now create new channel, using output variable for data storage */
//    dummy = p->arg;
    /* THIS NEEDS A LOCK BUT DOES NOT EXIST YET */
    /* lock = csoundGetChannelLock(csound, (char*) p->iname->data); */
    /* csoundSpinLock(lock); */
    err = create_new_channel(csound, (char*) p->iname->data, type);

    /* csoundSpinLock(lock); */
    if (err)
        return print_chn_err(p, err);

    /* Now we need to find the channel entry */
    chn = find_channel(csound, (char*) p->iname->data);
    /* free the allocated memory (we will not use it) */
    csound->Free(csound, chn->data);
    /* point to the arg var */
    chn->data = p->arg;

    /* if control channel, set additional parameters */
    if ((type & CSOUND_CHANNEL_TYPE_MASK) != CSOUND_CONTROL_CHANNEL)
        return OK;
    // ***FIXME not used type = (int32_t)MYFLT2LRND(*(p->itype));
    hints.behav = CSOUND_CONTROL_CHANNEL_LIN;
    hints.dflt = *(p->idflt);
    hints.min = *(p->imin);
    hints.max = *(p->imax);
    hints.x = hints.y = hints.width = hints.height = 0;
    hints.attributes = NULL;
    err = csoundSetControlChannelHints(csound, (char*) p->iname->data, hints);
    if (LIKELY(!err))
        return OK;
    if (err == CSOUND_MEMORY)
        return print_chn_err(p, err);
    return csound->InitError(csound, Str("invalid channel parameters"));

    arg_err:
    return csound->InitError(csound, Str("invalid export variable"));
}

/* returns all parameters of a channel */

int32_t chnparams_opcode_init(CSOUND *csound, CHNPARAMS_OPCODE *p)
{
    MYFLT *dummy;
    int32_t   err;

    /* all values default to zero... */
    *(p->itype)    = FL(0.0);
    *(p->imode)    = FL(0.0);
    *(p->ictltype) = FL(0.0);
    *(p->idflt)    = FL(0.0);
    *(p->imin)     = FL(0.0);
    *(p->imax)     = FL(0.0);
    err = csoundGetChannelPtr(csound, (void **) &dummy, (char*) p->iname->data, 0);
    /* ...if channel does not exist */
    if (err <= 0)
        return OK;
    /* type (control/audio/string) */
    *(p->itype) = (MYFLT) (err & 15);
    /* mode (input and/or output) */
    *(p->imode) = (MYFLT) ((err & 48) >> 4);
    /* check for control channel parameters */
    if ((err & 15) == CSOUND_CONTROL_CHANNEL) {
        controlChannelHints_t hints;
        err = csoundGetControlChannelHints(csound, (char*) p->iname->data, &hints);
        if (UNLIKELY(err > 0))
            *(p->ictltype) = (MYFLT) err;
        *(p->ictltype) = hints.behav;
        *(p->idflt) = hints.dflt;
        *(p->imin) = hints.min;
        *(p->imax) = hints.max;
    }
    return OK;
}

/* ********************************************************************** */
/* *************** SENSING ********************************************** */
/* ********************************************************************** */

int32_t sensekey_perf(CSOUND *csound, KSENSE *p)
{
    int32_t     keyCode = 0;
    int32_t     retval;

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

            if (retval>0) {
                char    ch = '\0';
                int32_t n=0;
                if (UNLIKELY((n=(int32_t)read(0, &ch, 1))<0)) {
                    csound->PerfError(csound, &(p->h),
                                      Str("read failure in sensekey\n"));
                    return NOTOK;
                }
                //if n==0 then EOF which we treat as empty
                if(n==0) ch = '\0';
                keyCode = (int32_t)((unsigned char) ch);
                /* FD_ISSET(0, &rfds) will be true. */
            }
            else if (retval<0) perror(Str("sensekey error:"));
#else
            unsigned char ch = (unsigned char) '\0';
#  ifdef WIN32
        if (_kbhit())
          ch = (unsigned char) _getch();
#  else
        ch = (unsigned char) getchar();
#  endif
        keyCode = (int32_t)ch;
#endif
        }
        else if (csound->inChar_ > 0) {
            keyCode = csound->inChar_;
            csound->inChar_ = 0;
        }
        if (p->evtbuf != -1) {
            int32_t     tmp = keyCode;
            keyCode = p->evtbuf;
            tmp = (keyCode < 0 ? tmp : (-1 - keyCode));
            p->evtbuf = (tmp != 0 ? tmp : -1);
        }
        // *** Cannot see point of next 2 lines *** JPff
        /* else if (p->OUTOCOUNT>1 && p->keyDown != NULL) */
        /*   p->evtbuf = -1 - keyCode; */
        if (keyCode < 0)
            keyCode = 65535 - keyCode;
    }
    else if (retval < 0) {
        keyCode = 0;
    }
    *(p->ans) = (MYFLT) ((keyCode & (int32_t)0xFFFF) ?
                         (keyCode & (int32_t)0xFFFF) : -1);
    if (p->OUTOCOUNT>1 && p->keyDown != NULL)
        *(p->keyDown) = (MYFLT) ((keyCode > 0 && keyCode < 65536) ? 1 : 0);

    return OK;
}


/* k-rate and string i/o opcodes */
/* invalue and outvalue are used with the csoundAPI */
/*     ma++ ingalls      matt@sonomatics.com */

int32_t kinval(CSOUND *csound, INVAL *p)
{
    if (csound->InputChannelCallback_) {
        csound->InputChannelCallback_(csound,
                                      (char*) p->channelName.auxp,
                                      p->value, p->channelType);
    }
    else
        *(p->value) = FL(0.0);

    return OK;
}

int32_t kinvalS(CSOUND *csound, INVAL *p)
{

    if (csound->InputChannelCallback_) {
        csound->InputChannelCallback_(csound,
                                      (char*) p->channelName.auxp,
                                      ((STRINGDAT *)p->value)->data,
                                      p->channelType);

    }
    else {
        ((STRINGDAT *)p->value)->data[0]  = '\0';
    }

    return OK;
}


int32_t invalset_string_S(CSOUND *csound, INVAL *p)
{
    int32_t   err;
    int32_t type;
    STRINGDAT *out = (STRINGDAT *) p->value;

    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp,
                              type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    if (out->data == NULL || out->size < 256) {
        if(out->data != NULL)
            csound->Free(csound, out->data);
        out->data = csound->Calloc(csound, 256);
        out->size = 256;
    }

    /* grab input now for use during i-pass */
    kinvalS(csound, p);
    if (!csound->InputChannelCallback_) {
        csound->Warning(csound,Str("InputChannelCallback not set."));
    }
    return OK;
}

int32_t invalset_S(CSOUND *csound, INVAL *p)
{
    int32_t   err;
    int32_t type;

    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp,
                              type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* grab input now for use during i-pass */
    kinval(csound, p);
    if (!csound->InputChannelCallback_) {
        csound->Warning(csound,Str("InputChannelCallback not set."));
    }
    return OK;
}

int32_t invalsetgo(CSOUND *csound, INVAL *p)
{
    int32_t ans = invalset(csound, p);
    if (ans==OK) ans = kinval(csound, p);
    return ans;
}

int32_t invalsetSgo(CSOUND *csound, INVAL *p)
{
    int32_t ans = invalset_S(csound, p);
    if (ans==OK) ans = kinval(csound, p);
    return ans;
}


int32_t invalset_string(CSOUND *csound, INVAL *p)
{
    int32_t   err;
    int32_t type;

    /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 64, &p->channelName);
    snprintf((char*) p->channelName.auxp, 64, "%d", (int32_t)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp,
                              type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* grab input now for use during i-pass */
    kinvalS(csound, p);
    if (!csound->InputChannelCallback_) {
        csound->Warning(csound,Str("InputChannelCallback not set."));
    }
    return OK;
}


int32_t invalset(CSOUND *csound, INVAL *p)
{
    int32_t   err;
    int32_t type;

    /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 32, &p->channelName);
    snprintf((char*) p->channelName.auxp, 32, "%d", (int32_t)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp,
                              type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* grab input now for use during i-pass */
    kinval(csound, p);
    if (!csound->InputChannelCallback_) {
        csound->Warning(csound,Str("InputChannelCallback not set."));
    }
    return OK;
}



int32_t koutvalS(CSOUND *csound, OUTVAL *p)
{
    char    *chan = (char*)p->channelName.auxp;

    if (csound->OutputChannelCallback_) {
        csound->OutputChannelCallback_(csound, chan,
                                       ((STRINGDAT *)p->value)->data,
                                       p->channelType);
    }
    return OK;
}

int32_t koutval(CSOUND *csound, OUTVAL *p)
{
    char    *chan = (char*)p->channelName.auxp;

    if (csound->OutputChannelCallback_) {
        csound->OutputChannelCallback_(csound, chan, p->value, p->channelType);
        *((MYFLT *) p->channelptr) = *(p->value);
    }

    return OK;
}

int32_t outvalset_string_S(CSOUND *csound, OUTVAL *p)
{
    int32_t type, err;
    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);


    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp, type);

    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* send output now for use during i-pass */
    koutvalS(csound, p);
    if (!csound->OutputChannelCallback_) {
        csound->Warning(csound,Str("OutputChannelCallback not set."));
    }

    return OK;
}



int32_t outvalset_S(CSOUND *csound, OUTVAL *p)
{
    int32_t type, err;
    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* send output now for use during i-pass */
    koutval(csound, p);
    if (!csound->OutputChannelCallback_) {
        csound->Warning(csound,Str("OutputChannelCallback not set."));
    }

    return OK;
}


int32_t outvalset_string(CSOUND *csound, OUTVAL *p)
{
    int32_t type, err;

    /* convert numerical channel to string name */
    if(p->channelName.auxp == NULL)
        csound->AuxAlloc(csound, 32, &p->channelName);
    snprintf((char*)p->channelName.auxp,  32, "%d",
             (int32_t)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (void **) &(p->channelptr),
                              (char*) p->channelName.auxp, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* send output now for use during i-pass */
    koutvalS(csound, p);
    if (!csound->OutputChannelCallback_) {
        csound->Warning(csound,Str("OutputChannelCallback not set."));
    }

    return OK;
}

int32_t outvalset(CSOUND *csound, OUTVAL *p)
{
    int32_t type, err;

    /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 64, &p->channelName);
    snprintf((char*)p->channelName.auxp,  64, "%d",
             (int32_t)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound,(void **)  &(p->channelptr),
                              (char*) p->channelName.auxp, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);

    /* send output now for use during i-pass */
    koutval(csound, p);
    if (!csound->OutputChannelCallback_) {
        csound->Warning(csound,Str("OutputChannelCallback not set."));
    }

    return OK;
}

int32_t outvalsetgo(CSOUND *csound, OUTVAL *p)
{
    int32_t ans = outvalset(csound,p);
    // if (ans==OK) ans = koutval(csound,p);
    return ans;
}

int32_t outvalsetSgo(CSOUND *csound, OUTVAL *p)
{
    int32_t ans = outvalset_S(csound,p);
    // if (ans==OK) ans = koutval(csound,p);
    return ans;
}

/* ARRAY channels  implementation - VL 7.08.24 */
static inline void copy_array(CSOUND *csound,
                       ARRAYDAT *out, const ARRAYDAT *in, spin_lock_t *lock) {
   csoundSpinLock(lock);
   CS_VAR_TYPE_ARRAY.copyValue(csound, &CS_VAR_TYPE_ARRAY, out, in, NULL); 
   csoundSpinUnLock(lock);
}

static int32_t init_chn_array(CSOUND* csound, CHNGET* p, int32_t type) {
    int32_t   err;
    ARRAYDAT *adat = (ARRAYDAT *) p->arg, *adat_chn;
    err = csoundGetChannelPtr(csound, (void **)&(p->fp),
                              (char*) p->iname->data,
                              CSOUND_ARRAY_CHANNEL | type);
    if (LIKELY(!err)) {
     p->lock =   (spin_lock_t *)
          csoundGetChannelLock(csound, (char*) p->iname->data);
        strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
    } else return csound->InitError(csound, "could not find channel\n");
    adat_chn = (ARRAYDAT *) p->fp;
    if(adat->data == NULL) {
      if(adat_chn->data == NULL)
      return csound->InitError(csound, "array channel not allocated\n");
      else tabinit_like(csound, adat, adat_chn);
      }
    
    if(adat_chn->data == NULL) {
      if(adat->data == NULL)
      return csound->InitError(csound, "array variable not allocated\n");
      else tabinit_like(csound, adat_chn, adat);
     }
   return OK;
}

int32_t array_perf_check(CSOUND* csound, CHNGET* p, int32_t type) {
    if (strncmp(p->chname, p->iname->data, MAX_CHAN_NAME)
        || !strcmp(p->iname->data, ""))
    {
        int32_t err = csoundGetChannelPtr(csound, (void **)&(p->fp),
                                          (char*) p->iname->data,
                                          CSOUND_ARRAY_CHANNEL |
                                          type);
        if (err==0){
            p->lock = (spin_lock_t*)
              csoundGetChannelLock(csound, (char*) p->iname->data);
            strNcpy(p->chname, p->iname->data, MAX_CHAN_NAME);
        }
        else {
            print_chn_err_perf(p, err);
            return NOTOK;
        }
    } 
    return OK;
}

/* receive ARRAYDAT from bus at performance time */
static int32_t chnget_opcode_perf_ARRAY(CSOUND* csound, CHNGET* p) 
{
  if(array_perf_check(csound, p, CSOUND_INPUT_CHANNEL) == OK) {
    copy_array(csound, (ARRAYDAT *) p->arg,  (ARRAYDAT *) p->fp, p->lock);
    return OK;
  }
  else return NOTOK;
}

/* init routine for chnget opcode (array data) */
int32_t chnget_opcode_init_ARRAY(CSOUND *csound, CHNGET *p)
{
  if(init_chn_array(csound, p, CSOUND_INPUT_CHANNEL) == OK) {
    ARRAYDAT *adat = (ARRAYDAT *) p->arg;
    if(adat->arrayType == &CS_VAR_TYPE_I) {
      copy_array(csound, adat, (ARRAYDAT *) p->fp, p->lock);
    } else 
    p->h.perf = (SUBR) chnget_opcode_perf_ARRAY;
    return OK;
  }
  else return NOTOK;
}

/* send ARRAYDAT to bus at performance time */
static int32_t chnset_opcode_perf_ARRAY(CSOUND* csound, CHNGET* p) 
{
  if(array_perf_check(csound, p, CSOUND_OUTPUT_CHANNEL) == OK) {
    copy_array(csound, (ARRAYDAT *) p->fp,  (ARRAYDAT *) p->arg, p->lock);
    return OK;
  }
  else return NOTOK;
}

/* init routine for chnset opcode (array data) */
int32_t chnset_opcode_init_ARRAY(CSOUND *csound, CHNGET *p)
{
  if(init_chn_array(csound, p, CSOUND_OUTPUT_CHANNEL) == OK) {
    ARRAYDAT *adat = (ARRAYDAT *) p->arg;
    if(adat->arrayType == &CS_VAR_TYPE_I) {
      copy_array(csound, (ARRAYDAT *) p->fp, adat, p->lock);
    } else 
    p->h.perf = (SUBR) chnset_opcode_perf_ARRAY;
    return OK;
  }
  else return NOTOK;
}

/* chn opcode (array channel) */
int32_t chn_opcode_init_ARRAY(CSOUND *csound, CHN_OPCODE_ARRAY *p)
{
    ARRAYDAT *adat;
    int32_t   type, mode, err, siz = 0, i;


    mode = (int32_t) MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
        return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_ARRAY_CHANNEL;
    if (mode & 1)
        type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
        type |= CSOUND_OUTPUT_CHANNEL;
    err = csoundGetChannelPtr(csound, (void **) &adat, (char*) p->iname->data, type);
    if (UNLIKELY(err))
        return print_chn_err(p, err);
    adat->dimensions = (int32_t) p->idim->sizes[0];
    adat->sizes = (int32_t *) csound->Calloc(csound,
                                           adat->dimensions*sizeof(int32_t));
    siz = (adat->sizes[0] = (int32_t) MYFLT2LRND(p->idim->data[0]));
    for(i = 1; i < adat->dimensions; i++)
      siz *= (adat->sizes[i] = (int32_t) MYFLT2LRND(p->idim->data[i]));
  
    adat->arrayType = (CS_TYPE *)
      csoundGetTypeWithVarTypeName(csound->typePool, p->type->data);
    tabinit(csound, adat, siz, &(p->h));
    return OK;
}

/* clear an array channel to zero at performance time */
static int32_t chnclear_opcode_perf_ARRAY(CSOUND *csound, CHNCLEAR *p)
{
    int32_t i, n=p->INCOUNT;
    IGN(csound);
    for (i=0; i<n; i++) {
        ARRAYDAT *adat = (ARRAYDAT*) p->fp[i];
        csoundSpinLock(p->lock[i]);
        memset(adat->data, 0, adat->allocated); 
        csoundSpinUnLock(p->lock[i]);
    }
    return OK;
}

/* init routine for chnclear opcode (array channel) */
int32_t chnclear_opcode_init_ARRAY(CSOUND *csound, CHNCLEAR *p)
{
    int32_t   err;
    int32_t   i, n = (int32_t)p->INCOUNT;
    for (i=0; i<n; i++) {
      err = csoundGetChannelPtr(csound, (void **) &(p->fp[i]),
                                (char*) p->iname[i]->data,
                                  CSOUND_ARRAY_CHANNEL | CSOUND_OUTPUT_CHANNEL);
        if (LIKELY(!err)) {
            p->lock[i] = (spin_lock_t *)
              csoundGetChannelLock(csound,(char*) p->iname[i]->data);
        }
        else return print_chn_err(p, err);
    }
    p->h.perf = (SUBR) chnclear_opcode_perf_ARRAY;
    return OK;
}

/* Channel API functions - threadsafe
   NB: the following do not depend on API_lock
   therefore do not need to be in the message queue
*/
MYFLT csoundGetControlChannel(CSOUND *csound, const char *name, int32_t *err)
{
  MYFLT *pval;
  int32_t err_;
  union {
    MYFLT d;
    MYFLT_INT_TYPE i;
  } x;
  x.d = FL(0.0);
  if (UNLIKELY(strlen(name) == 0)) return FL(.0);
  if ((err_ = csoundGetChannelPtr(csound, (void **) &pval, name,
                                  CSOUND_CONTROL_CHANNEL |
                                  CSOUND_OUTPUT_CHANNEL))
      == CSOUND_SUCCESS) {
#if defined(MSVC)
    x.i = InterlockedExchangeAdd64((MYFLT_INT_TYPE *)pval, 0);
#elif defined(HAVE_ATOMIC_BUILTIN)
    x.i = __atomic_load_n((MYFLT_INT_TYPE *)pval, __ATOMIC_SEQ_CST);
#else
    x.d = *pval;
#endif
  }
  if (err) {
    *err = err_;
  }
  return x.d;
}

void csoundSetControlChannel(CSOUND *csound, const char *name, MYFLT val){
  MYFLT *pval;
#if defined(MSVC) || defined(HAVE_ATOMIC_BUILTIN)
  union {
    MYFLT d;
    MYFLT_INT_TYPE i;
  } x;
  x.d = val;
#endif
  if (csoundGetChannelPtr(csound, (void **) &pval, name,
                          CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS)

#if defined(MSVC)
    InterlockedExchange64((MYFLT_INT_TYPE *)pval, x.i);
#elif defined(HAVE_ATOMIC_BUILTIN)
    __atomic_store_n((MYFLT_INT_TYPE *)pval, x.i, __ATOMIC_SEQ_CST);
#else
  {
    spin_lock_t *lock = (spin_lock_t *)
      csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    *pval  = val;
    csoundSpinUnLock(lock);
  }
#endif
}

void csoundGetAudioChannel(CSOUND *csound, const char *name,
                           MYFLT *samples)
{

  MYFLT  *psamples;
  if (strlen(name) == 0) return;
  if (csoundGetChannelPtr(csound, (void **) &psamples, name,
                          CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL)
      == CSOUND_SUCCESS) {
    spin_lock_t *lock = (spin_lock_t *)csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    memcpy(samples, psamples, csoundGetKsmps(csound)*sizeof(MYFLT));
    csoundSpinUnLock(lock);
  }
}

void csoundSetAudioChannel(CSOUND *csound, const char *name,
                           const MYFLT *samples)
{
  MYFLT  *psamples;
  if (csoundGetChannelPtr(csound, (void **) &psamples, name,
                          CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS){
    spin_lock_t *lock = (spin_lock_t *)csoundGetChannelLock(csound, (char*) name);
    csoundSpinLock(lock);
    memcpy(psamples, samples, csoundGetKsmps(csound)*sizeof(MYFLT));
    csoundSpinUnLock(lock);
  }
}

void csoundSetStringChannel(CSOUND *csound, const char *name,
                            const char *string)
{
  STRINGDAT* stringdat;

  if (csoundGetChannelPtr(csound, (void **) &stringdat, name,
                          CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL)
      == CSOUND_SUCCESS){
    size_t   size = stringdat->size; //csoundGetChannelDatasize(csound, name);
    spin_lock_t *lock = (spin_lock_t *) csoundGetChannelLock(csound, (char*) name);

    if (lock != NULL) {
      csoundSpinLock(lock);
    }

    if (strlen(string) + 1 > (uint32_t) size) {
      if (stringdat->data != NULL) csound->Free(csound,stringdat->data);
      stringdat->data = cs_strdup(csound, string);
      stringdat->size = strlen(string) + 1;
      //set_channel_data_ptr(csound,name,(void*)pstring, strlen(string)+1);
    } else {
      strcpy((char *) stringdat->data, string);
    }

    if (lock != NULL) {
      csoundSpinUnLock(lock);
    }
  }
}

void csoundGetStringChannel(CSOUND *csound, const char *name,
                            char *string)
{
  STRINGDAT  *pstring;
  char *chstring;
  int32_t n2;
  if (strlen(name) == 0) return;
  if (csoundGetChannelPtr(csound, (void **) &pstring, name,
                          CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)
      == CSOUND_SUCCESS){
    spin_lock_t *lock = (spin_lock_t *) csoundGetChannelLock(csound, (char*) name);
    chstring = pstring->data;
    if (lock != NULL)
      csoundSpinLock(lock);
    if (string != NULL && chstring != NULL) {
      n2 = (int32_t) strlen(chstring);
      strNcpy(string,chstring, n2+1);
    }
    if (lock != NULL)
      csoundSpinUnLock(lock);
  }
}

PUBLIC int32_t csoundSetPvsChannel(CSOUND *csound, const char *name,
                               const PVSDAT *fin)
{
  PVSDAT *f;
  if (LIKELY(csoundGetChannelPtr(csound, (void **) &f, name,
                                 CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL)
             == CSOUND_SUCCESS)){
    spin_lock_t *lock = (spin_lock_t *)
      csoundGetChannelLock(csound, name);

    csoundSpinLock(lock);
    if (f->frame.auxp == NULL || f->N < fin->N) 
       csound->AuxAlloc(csound, fin->frame.size, &f->frame);
    memcpy(f, fin, sizeof(PVSDAT)-sizeof(AUXCH));
    if (fin->frame.auxp != NULL)
      memcpy(f->frame.auxp, fin->frame.auxp, fin->frame.size);
    csoundSpinUnLock(lock);
  } else {
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

PUBLIC int32_t csoundGetPvsChannel(CSOUND *csound, const char *name,
                               PVSDAT *fout)
{
  PVSDAT *f;
  if (UNLIKELY(csoundGetChannelPtr(csound, (void **) &f, name,
                                   CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL)
               == CSOUND_SUCCESS)){
    spin_lock_t *lock = (spin_lock_t *)
      csoundGetChannelLock(csound, name);
    if (UNLIKELY(f == NULL)) return CSOUND_ERROR;
    csoundSpinLock(lock);
    memcpy(fout, f, sizeof(PVSDAT)-sizeof(AUXCH));
    if (fout->frame.auxp != NULL && f->frame.auxp != NULL)
      memcpy(fout->frame.auxp, f->frame.auxp, sizeof(float)*(fout->N));
    csoundSpinUnLock(lock);
  } else {
    return CSOUND_ERROR;
  }
  return CSOUND_SUCCESS;
}

PUBLIC ARRAYDAT *csoundInitArrayChannel(CSOUND *csound, const char *name,
                                        const char *type, int32_t dimensions,
                                        const int32_t *sizes) {
  int32_t i, siz = 0, err;
  ARRAYDAT *adat;

  err = csoundGetChannelPtr(csound, (void **) &adat, name,
                            CSOUND_ARRAY_CHANNEL |
                            CSOUND_INPUT_CHANNEL |
                            CSOUND_OUTPUT_CHANNEL);
  
  if(err != CSOUND_SUCCESS) return NULL;
  
  if(adat->data == NULL) {
  adat->dimensions = dimensions;
  adat->sizes = (int32_t *) csound->Calloc(csound,
                                           dimensions*sizeof(int32_t));
  siz = (adat->sizes[0] = sizes[0]);
  for(i = 0; i < adat->dimensions; i++)
    siz *= (adat->sizes[i] = sizes[i]);
  
  adat->arrayType = (CS_TYPE *)
    csoundGetTypeWithVarTypeName(csound->typePool, type);
  tabinit(csound, adat, siz, NULL);
 }
  return adat;
}

PUBLIC int32_t csoundArrayDataDimensions(const ARRAYDAT *adat) {
  return adat->dimensions;
}

PUBLIC const char *csoundArrayDataType(const ARRAYDAT *adat) {
  return adat->arrayType->varTypeName; 
}

PUBLIC const int32_t *csoundArrayDataSizes(const ARRAYDAT *adat){
  return adat->sizes;
}

PUBLIC void csoundSetArrayData(ARRAYDAT *adat,
                               const void* data) {
  size_t siz = 0, i;
  for(i = 0; i < adat->dimensions; i++)
    siz += adat->sizes[i];
  memcpy(adat->data, data, siz*adat->arrayMemberSize);
}
  
PUBLIC const void *csoundGetArrayData(const ARRAYDAT *adat) {
  return adat->data;
}

PUBLIC void csoundLockChannel(CSOUND *csound, const char *channel) {
  csoundSpinLock((spin_lock_t *) csoundGetChannelLock(csound, (char*) channel));
}

PUBLIC void csoundUnlockChannel(CSOUND *csound, const char *channel) {
  csoundSpinUnLock((spin_lock_t *)csoundGetChannelLock(csound, (char*) channel));
}
