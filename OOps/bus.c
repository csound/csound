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
         Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
         02111-1307 USA
*/

/*                      BUS.C           */
#include "csoundCore.h"
#include <setjmp.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#ifdef NACL
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



int chani_opcode_perf_k(CSOUND *csound, CHNVAL *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, p->h.insdshead,Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &val, chan_name,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("chani error %d:"
                                   "channel not found or not right type"), err);
    *(p->r) = *val;
    return OK;
}

int chano_opcode_perf_k(CSOUND *csound, CHNVAL *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound,p->h.insdshead,Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &val, chan_name,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);

    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("chano error %d:"
                                   "channel not found or not right type"), err);
    *val = *(p->r);
    return OK;
}

int chani_opcode_perf_a(CSOUND *csound, CHNVAL *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    char chan_name[16];
    int   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, p->h.insdshead,Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &val, chan_name,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("chani error %d:"
                                   "channel not found or not right type"), err);
    if (UNLIKELY(offset)) memset(p->r, '\0', offset * sizeof(MYFLT));
    memcpy(&p->r[offset], &val[offset],
           sizeof(MYFLT) * (CS_KSMPS-offset-early));
    if (UNLIKELY(early))
      memset(&p->r[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    return OK;
}

int chano_opcode_perf_a(CSOUND *csound, CHNVAL *p)
{
    int     n = (int)MYFLT2LRND(*(p->a));
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    char chan_name[16];
    int   err;
    MYFLT *val;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, p->h.insdshead,Str("chani: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &val, chan_name,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("chano error %d:"
                                   "channel not found or not right type"), err);

    if (UNLIKELY(offset)) memset(&val, '\0', offset * sizeof(MYFLT));
    memcpy(&val[offset], &p->r[offset],
           sizeof(MYFLT) * (CS_KSMPS-offset-early));

    if (UNLIKELY(early))
      memset(&val[CS_KSMPS-early], '\0', early * sizeof(MYFLT));
    return OK;
}

int pvsin_init(CSOUND *csound, FCHAN *p)
{
    int N;
    MYFLT *pp;
    PVSDATEXT *f;
    int     n = (int)MYFLT2LRND(*(p->a));
    char name[16];
    snprintf(name, 16, "%i", n);
    if (csoundGetChannelPtr(csound, &pp, name,
                           CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL)
            == CSOUND_SUCCESS){
        int    *lock =
                csoundGetChannelLock(csound, name);
        f = (PVSDATEXT *) pp;
        csoundSpinLock(lock);
        memcpy(&(p->init), f, sizeof(PVSDAT)-sizeof(AUXCH));
        csoundSpinUnLock(lock);
    }

    N = p->init.N = (int32)(*p->N ? *p->N : p->init.N);
    p->init.overlap = (int32) (*p->overlap ? *p->overlap : p->init.overlap);
    p->init.winsize = (int32) (*p->winsize ? *p->winsize : p->init.winsize);
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
    PVSDAT *fout = p->r;
    int     n = (int)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int   err, size, *lock;
    PVSDATEXT *fin;
    MYFLT      *pp;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, p->h.insdshead,Str("pvsin: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &pp, chan_name,
                              CSOUND_PVS_CHANNEL | CSOUND_INPUT_CHANNEL);
    fin = (PVSDATEXT *) pp;
    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("pvsin error %d:"
                                   "channel not found or not right type"), err);

    size = fin->N < fout->N ? fin->N : fout->N;
    lock = csoundGetChannelLock(csound, chan_name);
    csoundSpinLock(lock);
    memcpy(fout, fin, sizeof(PVSDAT)-sizeof(AUXCH));
    //printf("fout=%p fout->frame.auxp=%p fin=%p fin->frame=%p\n",
    //       fout, fout->frame.auxp, fin, fin->frame);
    if(fin->frame != NULL)
       memcpy(fout->frame.auxp, fin->frame, sizeof(float)*(size+2));
    else memset(fout->frame.auxp, 0, sizeof(float)*(size+2));
    csoundSpinUnLock(lock);
    return OK;
}

int pvsout_init(CSOUND *csound, FCHAN *p)
{
    PVSDAT *fin = p->r;
    MYFLT *pp;
    PVSDATEXT *f;
    int     n = (int)MYFLT2LRND(*(p->a));
    char name[16];

    snprintf(name, 16, "%i", n);
    if (csoundGetChannelPtr(csound, &pp, name,
                           CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL)
            == CSOUND_SUCCESS){
        int    *lock =
                csoundGetChannelLock(csound, name);
        f = (PVSDATEXT *) pp;
        csoundSpinLock(lock);
        if(f->frame == NULL) {
          f->frame = csound->Calloc(csound, sizeof(float)*(fin->N+2));
        } else if(f->N < fin->N) {
          f->frame = csound->ReAlloc(csound, f->frame, sizeof(float)*(fin->N+2));
        }
        memcpy(f, fin, sizeof(PVSDAT)-sizeof(AUXCH));
        csoundSpinUnLock(lock);
    }
    return OK;
}


int pvsout_perf(CSOUND *csound, FCHAN *p)
{

    PVSDAT *fin = p->r;
    int     n = (int)MYFLT2LRND(*(p->a));
    char chan_name[16];
    int   err, size, *lock;
    PVSDATEXT *fout;
    MYFLT *pp;

    if (UNLIKELY(n < 0))
      return csound->PerfError(csound, p->h.insdshead,Str("pvsout: invalid index"));

    snprintf(chan_name, 16, "%i", n);
    err = csoundGetChannelPtr(csound, &pp, chan_name,
                              CSOUND_PVS_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    fout = (PVSDATEXT *) pp;
    if (UNLIKELY(err))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("pvsout error %d:"
                                   "channel not found or not right type"), err);

    lock = csoundGetChannelLock(csound, chan_name);
    csoundSpinLock(lock);
    size = fin->N < fout->N ? fin->N : fout->N;
    memcpy(fout, fin, sizeof(PVSDAT)-sizeof(AUXCH));
    if(fout->frame != NULL)
       memcpy(fout->frame, fin->frame.auxp, sizeof(float)*(size+2));
    csoundSpinUnLock(lock);
    return OK;
}
/* ======================================================================== */

/* "chn" opcodes and bus interface by Istvan Varga */

static int delete_channel_db(CSOUND *csound, void *p)
{
    CONS_CELL *head, *values;

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

void set_channel_data_ptr(CSOUND *csound, const char *name, void *ptr, int newSize)
{
    find_channel(csound, name)->data = (MYFLT *) ptr;
    find_channel(csound, name)->datasize = newSize;
}

#define INIT_STRING_CHANNEL_DATASIZE 256

static CS_NOINLINE CHNENTRY *alloc_channel(CSOUND *csound,
                                           const char *name, int type)
{
    CHNENTRY      *pp;
    int           dsize = 0;
    switch (type & CSOUND_CHANNEL_TYPE_MASK) {
      case CSOUND_CONTROL_CHANNEL:
        dsize = sizeof(MYFLT);
        break;
      case CSOUND_AUDIO_CHANNEL:
        dsize = ((int)sizeof(MYFLT) * csound->ksmps);
        break;
      case CSOUND_STRING_CHANNEL:
        dsize = (sizeof(STRINGDAT));
        break;
      case CSOUND_PVS_CHANNEL:
        dsize = (sizeof(PVSDATEXT));
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

#ifndef MACOSX
#if defined(HAVE_PTHREAD_SPIN_LOCK)
    pthread_spin_init(&(pp->theLock), PTHREAD_PROCESS_PRIVATE);
    pp->lock = &(pp->theLock);
#endif
#endif
    pp->datasize = dsize;

    return (CHNENTRY*) pp;
}

static CS_NOINLINE int create_new_channel(CSOUND *csound, const char *name,
                                          int type)
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


PUBLIC int csoundGetChannelPtr(CSOUND *csound,
                               MYFLT **p, const char *name, int type)
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

PUBLIC int csoundGetChannelDatasize(CSOUND *csound, const char *name){

    CHNENTRY  *pp;
    pp = find_channel(csound, name);
    if (pp == NULL) return 0;
    else {
      /* the reason for this is that if chnexport is
         used with strings, the datasize might become
         invalid */
      if ((pp->type & CSOUND_STRING_CHANNEL) == CSOUND_STRING_CHANNEL)
        return ((STRINGDAT*)pp->data)->size;
      return pp->datasize;
    }
}


PUBLIC int *csoundGetChannelLock(CSOUND *csound,
                                const char *name)
{
    CHNENTRY  *pp;

    if (UNLIKELY(name == NULL))
      return NULL;
    pp = find_channel(csound, name);
    if (pp) {
#ifndef MACOSX
#if defined(HAVE_PTHREAD_SPIN_LOCK)
      return (int*)pp->lock;
#else
      return &(pp->lock);
#endif
#else
      return &(pp->lock);
#endif
    }
    else return NULL;
}

static int cmp_func(const void *p1, const void *p2)
{
    return strcmp(((controlChannelInfo_t*) p1)->name,
                  ((controlChannelInfo_t*) p2)->name);
}

PUBLIC int csoundListChannels(CSOUND *csound, controlChannelInfo_t **lst)
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
    // TODO - should this be malloc or csound->Malloc?
    *lst = (controlChannelInfo_t*) malloc(n * sizeof(controlChannelInfo_t));
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
    return (int)n;
}

PUBLIC void csoundDeleteChannelList(CSOUND *csound, controlChannelInfo_t *lst)
{
    (void) csound;
    if (lst != NULL) free(lst);
}

PUBLIC int csoundSetControlChannelHints(CSOUND *csound, const char *name,
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
        hints.dflt = (MYFLT) ((int32) MYFLT2LRND(hints.dflt));
        hints.min  = (MYFLT) ((int32) MYFLT2LRND(hints.min));
        hints.max  = (MYFLT) ((int32) MYFLT2LRND(hints.max));
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

PUBLIC int csoundGetControlChannelHints(CSOUND *csound, const char *name,
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

int notinit_opcode_stub(CSOUND *csound, void *p)
{
  return csound->PerfError(csound, ((CHNGET *)p)->h.insdshead,
                           Str("%s: not initialised"),
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
#ifdef HAVE_ATOMIC_BUILTIN
    volatile union {
    MYFLT d;
    MYFLT_INT_TYPE i;
    } x;
    x.i = __sync_fetch_and_add((MYFLT_INT_TYPE *) p->fp, 0);
    *(p->arg) = x.d;
#else
    *(p->arg) = *(p->fp);
#endif
    return OK;
}

/* receive audio data from bus at performance time */

static int chnget_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;

    if(CS_KSMPS == (unsigned int) csound->ksmps) {
    csoundSpinLock(p->lock);
    if (UNLIKELY(offset)) memset(p->arg, '\0', offset);
    memcpy(&p->arg[offset], p->fp, sizeof(MYFLT)*(CS_KSMPS-offset-early));
    if (UNLIKELY(early))
      memset(&p->arg[CS_KSMPS-early], '\0', sizeof(MYFLT)*early);
    csoundSpinUnLock(p->lock);
    } else {
    csoundSpinLock(p->lock);
    if (UNLIKELY(offset)) memset(p->arg, '\0', offset);
    memcpy(&p->arg[offset], &(p->fp[offset+p->pos]),
           sizeof(MYFLT)*(CS_KSMPS-offset-early));
    if (UNLIKELY(early))
      memset(&p->arg[CS_KSMPS-early], '\0', sizeof(MYFLT)*early);
    p->pos+=CS_KSMPS;
    p->pos %= (csound->ksmps-offset);
    csoundSpinUnLock(p->lock);
    }

    return OK;
}

/* receive control value from bus at init time */

int chnget_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
#ifdef HAVE_ATOMIC_BUILTIN
    {
    union {
    MYFLT d;
    MYFLT_INT_TYPE i;
    } x;
    x.i = __sync_fetch_and_add((MYFLT_INT_TYPE *) p->fp, 0);
    *(p->arg) =  x.d;
    }
#else
    *(p->arg) = *(p->fp);
#endif
    return OK;
}

/* init routine for chnget opcode (control data) */

int chnget_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL);
    p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
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
    p->pos = 0;
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_INPUT_CHANNEL);
    p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);

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
    char *s = ((STRINGDAT *) p->arg)->data;
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
    p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);

    if (UNLIKELY(err))
      return print_chn_err(p, err);
    csoundSpinLock(p->lock);
    if(((STRINGDAT *) p->fp)->data != NULL) {
      if(((STRINGDAT *) p->fp)->size > ((STRINGDAT *) p->arg)->size) {
     if(s != NULL) csound->Free(csound, s);
    s = cs_strdup(csound,((STRINGDAT *) p->fp)->data);
    ((STRINGDAT *) p->arg)->data = s;
    ((STRINGDAT *) p->arg)->size = strlen(s) + 1;
      } else strcpy(((STRINGDAT *) p->arg)->data, ((STRINGDAT *) p->fp)->data);
    }
    csoundSpinUnLock(p->lock);
    return OK;
}

int chnget_opcode_perf_S(CSOUND *csound, CHNGET *p)
{
    int   err;
    char *s = ((STRINGDAT *) p->arg)->data;
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL);
     p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);

    if (UNLIKELY(err))
      return print_chn_err(p, err);

    if(s != NULL && ((STRINGDAT *) p->fp)->data != NULL &&
      strcmp(s, ((STRINGDAT *) p->fp)->data) == 0) return OK;

    csoundSpinLock(p->lock);

    if(((STRINGDAT *) p->fp)->data != NULL){
    if(((STRINGDAT *) p->arg)->size <= ((STRINGDAT *) p->fp)->size) {
    if(s != NULL) csound->Free(csound, s);
    s = cs_strdup(csound,((STRINGDAT *) p->fp)->data);
    ((STRINGDAT *) p->arg)->data = s;
    ((STRINGDAT *) p->arg)->size = strlen(s) + 1;
    }
    else strcpy (((STRINGDAT *) p->arg)->data, ((STRINGDAT *) p->fp)->data);
      }
    csoundSpinUnLock(p->lock);
    return OK;
}


/* send control value to bus at performance time */

static int chnset_opcode_perf_k(CSOUND *csound, CHNGET *p)
{
#ifdef HAVE_ATOMIC_BUILTIN
    union {
      MYFLT d;
      MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    __sync_lock_test_and_set((MYFLT_INT_TYPE *)(p->fp),x.i);
#else
    csoundSpinLock(p->lock);
    *(p->fp) = *(p->arg);
    csoundSpinUnLock(p->lock);
#endif
    return OK;
}

/* send audio data to bus at performance time */

static int chnset_opcode_perf_a(CSOUND *csound, CHNGET *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    if(CS_KSMPS == (unsigned int) csound->ksmps){
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

static int chnmix_opcode_perf(CSOUND *csound, CHNGET *p)
{
    uint32_t n = 0;
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

static int chnclear_opcode_perf(CSOUND *csound, CHNCLEAR *p)
{
    /* Need lock for the channel */
    csoundSpinLock(p->lock);
    memset(p->fp, 0, CS_KSMPS*sizeof(MYFLT)); /* Should this leave start? */
    csoundSpinUnLock(p->lock);
    return OK;
}

/* send control value to bus at init time */

int chnset_opcode_init_i(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (UNLIKELY(err))
      return print_chn_err(p, err);


#ifdef HAVE_ATOMIC_BUILTIN
    union {
      MYFLT d;
      MYFLT_INT_TYPE i;
    } x;
    x.d = *(p->arg);
    __sync_lock_test_and_set((MYFLT_INT_TYPE *)(p->fp),x.i);
#else
    {
      int *lock;       /* Need lock for the channel */
      p->lock = lock =
        csoundGetChannelLock(csound, (char*) p->iname->data);
      csoundSpinLock(lock);
      *(p->fp) = *(p->arg);
      csoundSpinUnLock(lock);
    }
#endif
    return OK;
}

/* init routine for chnset opcode (control data) */

int chnset_opcode_init_k(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
      p->h.opadr = (SUBR) chnset_opcode_perf_k;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnset opcode (audio data) */

int chnset_opcode_init_a(CSOUND *csound, CHNGET *p)
{
    int   err;
    p->pos = 0;
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (!err) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
      p->h.opadr = (SUBR) chnset_opcode_perf_a;
      return OK;
    }
    return print_chn_err(p, err);
}

/* init routine for chnmix opcode */

int chnmix_opcode_init(CSOUND *csound, CHNGET *p)
{
    int   err;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
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
    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_AUDIO_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    if (LIKELY(!err)) {
      p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
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
    char *s = ((STRINGDAT *) p->arg)->data;

    err = csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                              CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL);
    // size = csoundGetChannelDatasize(csound, p->iname->data);
    if (UNLIKELY(err))
      return print_chn_err(p, err);

    if (s==NULL) return NOTOK;
    p->lock = lock =
      csoundGetChannelLock(csound, (char*) p->iname->data);
    csoundSpinLock(lock);
    if (strlen(s) >= (unsigned int) ((STRINGDAT *)p->fp)->size) {
      if (((STRINGDAT *)p->fp)->data != NULL)
        csound->Free(csound, ((STRINGDAT *)p->fp)->data);
      ((STRINGDAT *)p->fp)->data = cs_strdup(csound, s);
      ((STRINGDAT *)p->fp)->size = strlen(s)+1;
      //set_channel_data_ptr(csound, p->iname->data,p->fp, strlen(s)+1);
    }
    else if(((STRINGDAT *)p->fp)->data != NULL)
            strcpy(((STRINGDAT *)p->fp)->data, s);
    csoundSpinUnLock(lock);

    return OK;
}

int chnset_opcode_perf_S(CSOUND *csound, CHNGET *p)
{
    int   err;
    int  *lock;
    char *s = ((STRINGDAT *) p->arg)->data;

    if ((err=csoundGetChannelPtr(csound, &(p->fp), (char*) p->iname->data,
                                 CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL)))
      return err;
    // size = csoundGetChannelDatasize(csound, p->iname->data);

    if (s==NULL) return NOTOK;
    if (((STRINGDAT *)p->fp)->data
        && strcmp(s, ((STRINGDAT *)p->fp)->data) == 0) return OK;

    p->lock = lock =
      csoundGetChannelLock(csound, (char*) p->iname->data);
    csoundSpinLock(lock);
    if (strlen(s) >= (unsigned int) ((STRINGDAT *)p->fp)->size) {
      if (((STRINGDAT *)p->fp)->data != NULL)
        csound->Free(csound, ((STRINGDAT *)p->fp)->data);
      ((STRINGDAT *)p->fp)->data = cs_strdup(csound, s);
      ((STRINGDAT *)p->fp)->size = strlen(s)+1;
      //set_channel_data_ptr(csound, p->iname->data,p->fp, strlen(s)+1);
    }
    else if(((STRINGDAT *)p->fp)->data != NULL)
            strcpy(((STRINGDAT *)p->fp)->data, s);
    csoundSpinUnLock(lock);
    //printf("%s \n", (char *)p->fp);
    return OK;
}

/* declare control channel, optionally with special parameters */

int chn_k_opcode_init(CSOUND *csound, CHN_OPCODE_K *p)
{
    MYFLT *dummy;
    int   type, mode, err;
    controlChannelHints_t hints;
    hints.attributes = NULL;
    hints.max = hints.min = hints.dflt = FL(0.0);
    hints.x = hints.y = hints.height = hints.width = 0;

    mode = (int)MYFLT2LRND(*(p->imode));
    if (UNLIKELY(mode < 1 || mode > 3))
      return csound->InitError(csound, Str("invalid mode parameter"));
    type = CSOUND_CONTROL_CHANNEL;
    if (mode & 1)
      type |= CSOUND_INPUT_CHANNEL;
    if (mode & 2)
      type |= CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname->data, type);
    if (err)
      return print_chn_err(p, err);
    hints.behav = CSOUND_CONTROL_CHANNEL_NO_HINTS;
    if ((int)MYFLT2LRND(*(p->itype)) == 1)
        hints.behav = CSOUND_CONTROL_CHANNEL_INT;
    else if ((int)MYFLT2LRND(*(p->itype)) == 2)
        hints.behav |= CSOUND_CONTROL_CHANNEL_LIN;
    else if ((int)MYFLT2LRND(*(p->itype)) == 3)
        hints.behav |= CSOUND_CONTROL_CHANNEL_EXP;
    if ((int)MYFLT2LRND(*(p->itype)) != 0) {
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
      p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
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
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname->data, type);
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
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname->data, type);
    if (UNLIKELY(err))
      return print_chn_err(p, err);
    p->lock = csoundGetChannelLock(csound, (char*) p->iname->data);
    return OK;
}

/* export new channel from global orchestra variable */

int chnexport_opcode_init(CSOUND *csound, CHNEXPORT_OPCODE *p)
{
    MYFLT       *dummy;
    const char  *argName;
    int         type = CSOUND_CONTROL_CHANNEL, mode, err;
    controlChannelHints_t hints;
    CHNENTRY *chn;

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
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname->data, 0);
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
    type = (int)MYFLT2LRND(*(p->itype));
    hints.behav = CSOUND_CONTROL_CHANNEL_LIN;
    hints.dflt = *(p->idflt);
    hints.min = *(p->imin);
    hints.max = *(p->imax);
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
    err = csoundGetChannelPtr(csound, &dummy, (char*) p->iname->data, 0);
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
          char    ch = '\0';
          if (UNLIKELY(read(0, &ch, 1)!=1)) {
            csound->PerfError(csound, p->h.insdshead,
                              Str("read failure in sensekey\n"));
            return NOTOK;
          }
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


/* k-rate and string i/o opcodes */
/* invalue and outvalue are used with the csoundAPI */
/*     ma++ ingalls      matt@sonomatics.com */

int kinval(CSOUND *csound, INVAL *p)
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

int kinvalS(CSOUND *csound, INVAL *p)
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


int invalset_string_S(CSOUND *csound, INVAL *p)
{
    int   err;
    int type;
    STRINGDAT *out = (STRINGDAT *) p->value;

    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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

int invalset_S(CSOUND *csound, INVAL *p)
{
    int   err;
    int type;

    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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

int invalsetgo(CSOUND *csound, INVAL *p)
{
    int ans = invalset(csound, p);
    if (ans==OK) ans = kinval(csound, p);
    return ans;
}

int invalsetSgo(CSOUND *csound, INVAL *p)
{
    int ans = invalset_S(csound, p);
    if (ans==OK) ans = kinval(csound, p);
    return ans;
}


int invalset_string(CSOUND *csound, INVAL *p)
{
    int   err;
    int type;

     /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 64, &p->channelName);
    snprintf((char*) p->channelName.auxp, 64, "%d", (int)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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


int invalset(CSOUND *csound, INVAL *p)
{
    int   err;
    int type;

     /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 32, &p->channelName);
    snprintf((char*) p->channelName.auxp, 32, "%d", (int)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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



int koutvalS(CSOUND *csound, OUTVAL *p)
{
    char    *chan = (char*)p->channelName.auxp;

    if (csound->OutputChannelCallback_) {
      csound->OutputChannelCallback_(csound, chan,
                                     ((STRINGDAT *)p->value)->data,
                                     p->channelType);
    }
    return OK;
}

int koutval(CSOUND *csound, OUTVAL *p)
{
    char    *chan = (char*)p->channelName.auxp;

    if (csound->OutputChannelCallback_) {
      csound->OutputChannelCallback_(csound, chan, p->value, p->channelType);
      *((MYFLT *) p->channelptr) = *(p->value);
    }

    return OK;
}

int outvalset_string_S(CSOUND *csound, OUTVAL *p)
{
    int type, err;
    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);


    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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



int outvalset_S(CSOUND *csound, OUTVAL *p)
{
    int type, err;
    const char  *s = ((STRINGDAT *)p->valID)->data;
    csound->AuxAlloc(csound, strlen(s) + 1, &p->channelName);
    strcpy((char*) p->channelName.auxp, s);

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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


int outvalset_string(CSOUND *csound, OUTVAL *p)
{
    int type, err;

    /* convert numerical channel to string name */
    if(p->channelName.auxp == NULL)
     csound->AuxAlloc(csound, 32, &p->channelName);
    snprintf((char*)p->channelName.auxp,  32, "%d",
            (int)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_S;
    type = CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound, (MYFLT **) &(p->channelptr),
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

int outvalset(CSOUND *csound, OUTVAL *p)
{
    int type, err;

    /* convert numerical channel to string name */
    csound->AuxAlloc(csound, 64, &p->channelName);
    snprintf((char*)p->channelName.auxp,  64, "%d",
            (int)MYFLT2LRND(*p->valID));

    p->channelType = &CS_VAR_TYPE_K;
    type = CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL;

    err = csoundGetChannelPtr(csound,(MYFLT **)  &(p->channelptr),
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

int outvalsetgo(CSOUND *csound, OUTVAL *p)
{
    int ans = outvalset(csound,p);
    if (ans==OK) ans = koutval(csound,p);
    return ans;
}

int outvalsetSgo(CSOUND *csound, OUTVAL *p)
{
    int ans = outvalset_S(csound,p);
    if (ans==OK) ans = koutval(csound,p);
    return ans;
}
