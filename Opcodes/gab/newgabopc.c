/*   Copyright (C) 2007 Gabriel Maldonado

     Csound is free software; you can redistribute it
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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

/* (Shouldn't there be global decl's for these?) */
#define INCR (0.001f)

/* -------------------------------------------------------------------- */

typedef struct {
  OPDS    h;
  MYFLT    *xfn, *outargs[VARGMAX];
  int32_t nargs;
  int64_t  pfn;
  MYFLT   *ftable;
} MTABLE1;


static int32_t  mtable1_set(CSOUND *csound, MTABLE1 *p) /* mtab by G.Maldonado */
{
  FUNC *ftp;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->xfn)) == NULL))
    return csound->InitError(csound, "%s", Str("vtable1: incorrect table number"));
  p->ftable = ftp->ftable;
  p->nargs = p->INOCOUNT-1;
  p->pfn = (int64_t) *p->xfn;
  return OK;
}

static int32_t  mtable1_k(CSOUND *csound, MTABLE1 *p)
{
  int32_t j, nargs = p->nargs;
  MYFLT **out = p->outargs;
  MYFLT *table;
  if (p->pfn != (int64_t)*p->xfn) {
    FUNC *ftp;
    if (UNLIKELY( (ftp = csound->FTFind(csound, p->xfn) ) == NULL))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("vtable1: incorrect table number"));
    p->pfn = (int64_t)*p->xfn;
    p->ftable = ftp->ftable;
  }
  table= p->ftable;
  for (j=0; j < nargs; j++)
    *out[j] =  table[j];
  return OK;
}

/* -------------------------------------------------------------------- */


#include "Opcodes/uggab.h"

/* -------------------------------------------------------------------- */

typedef struct  {
  OPDS    h;
  MYFLT   *out1, *out2, *amp, *freq, *kloop, *kend, *ift, *iphs;
  int64_t    tablen;
  MYFLT   fsr;
  MYFLT *ft; /*table */
  double  phs, fsrUPsr /* , looplength */;
  int64_t    phs_int;
} LPOSC_ST;

static int32_t lposc_stereo_set(CSOUND *csound, LPOSC_ST *p)
{
  FUNC *ftp;
  double  loop, end, looplength, fsr;
  if (UNLIKELY((ftp = csound->FTFind(csound, p->ift)) == NULL))
    return csound->InitError(csound, "%s", Str("invalid function"));
  if (UNLIKELY(!(fsr = ftp->gen01args.sample_rate))) {
    csound->Message(csound, "%s", Str("lposcil: no sample rate stored in function;"
                                      " assuming=sr\n"));
    p->fsr=CS_ESR;
  }
  p->fsrUPsr = fsr/CS_ESR;
  p->ft     = ftp->ftable;
  p->tablen = ftp->flen/2;
  /* changed from
     p->phs    = *p->iphs * p->tablen;   */
  if ((loop = *p->kloop) < 0) loop=0;/* gab */
  else if (loop > p->tablen-3) loop = p->tablen-3;
  if ((end = *p->kend) > p->tablen-1 ) end = p->tablen - 1;
  else if (end <= 2) end = 2;
  if (end < loop+2) end = loop + 2;
  looplength = end - loop;

  if (*p->iphs >= 0)
    p->phs_int = (int64_t) (p->phs = *p->iphs);

  while (p->phs >= end)
    p->phs_int = (int64_t) (p->phs -= looplength);
  return OK;
}

static int32_t lposca_stereo(CSOUND *csound, LPOSC_ST *p) /* stereo lposcinta */
{
  IGN(csound);
  double  *phs= &p->phs,   si= *p->freq * p->fsrUPsr;
  MYFLT   *out1 = p->out1, *out2 = p->out2, *amp=p->amp;
  MYFLT   *ft =  p->ft;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int32    loop, end, looplength /* = p->looplength */ ;
  if ((loop = (int32_t) *p->kloop) < 0) loop=0;/* gab */
  else if (loop > p->tablen-3) loop = (int32_t) (p->tablen-3);
  if ((end = (int32_t) *p->kend) > p->tablen-1 ) end = (int32_t) (p->tablen - 1);
  else if (end <= 2) end = 2;
  if (end < loop+2) end = loop + 2;
  looplength = end - loop;
  if (UNLIKELY(offset)) {
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    double fract;
    MYFLT *curr_samp1 = ft + (int64_t) *phs * 2;
    MYFLT *curr_samp2 = curr_samp1 +1;
    fract= *phs - (int64_t) *phs;
    out1[n] = amp[n] * (MYFLT)(*curr_samp1 +(*(curr_samp1+2)-*curr_samp1)*fract);
    out2[n] = amp[n] * (MYFLT)(*curr_samp2 +(*(curr_samp2+2)-*curr_samp2)*fract);
    *phs += si;
    while (*phs  >= end) *phs -= looplength;
    while (*phs  < loop) *phs += looplength;
  }
  return OK;
}

static int32_t lposca_stereo_no_trasp(CSOUND *csound, LPOSC_ST *p)
{    /* transposition is allowed only */
     /*in integer values (twice, three times etc.) so it is faster */
  IGN(csound);
  int64_t    *phs = &p->phs_int, si = (int64_t) *p->freq;
  MYFLT   *out1 = p->out1, *out2 = p->out2, *amp=p->amp;
  MYFLT   *ft =  p->ft;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  int64_t    loop, end, looplength /* = p->looplength */ ;
  if ((loop = (int64_t) *p->kloop) < 0) loop=0;/* gab */
  else if (loop > p->tablen-3) loop = p->tablen-3;
  if ((end = (int64_t) *p->kend) > p->tablen-1 ) end = p->tablen - 1;
  else if (end <= 2) end = 2;
  if (end < loop+2) end = loop + 2;
  looplength = end - loop;

  if (UNLIKELY(offset)) {
    memset(out1, '\0', offset*sizeof(MYFLT));
    memset(out2, '\0', offset*sizeof(MYFLT));
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&out1[nsmps], '\0', early*sizeof(MYFLT));
    memset(&out2[nsmps], '\0', early*sizeof(MYFLT));
  }
  for (n=offset; n<nsmps; n++) {
    MYFLT *curr_samp1 = ft + *phs * 2;
    out1[n] = amp[n] * (MYFLT) *curr_samp1 ;
    out2[n] = amp[n] * (MYFLT) *(curr_samp1+1) ;
    *phs += si;
    while (*phs  >= end) *phs -= looplength;
    while (*phs  < loop) *phs += looplength;
  }
  return OK;
}


/* -------------------------------------------------------------------- */

#include "vectorial.h"
typedef struct  {       /* gab d5*/
  OPDS    h;
  MYFLT   *out, *ktrig, *min, *max;
  MYFLT   lastvalue;
} TRANGERAND;

static int32_t trRangeRand(CSOUND *csound, TRANGERAND *p)
{ /* gab d5*/
  if (*p->ktrig)
    *p->out = p->lastvalue = randGab(csound) * (*p->max - *p->min) + *p->min;
  else
    *p->out = p->lastvalue;
  return OK;
}


/* -------------------------------------------------------------------- */
/* Note: this opcode has been left out because it is undocumented */

#if 0
typedef struct
{
  OPDS    h;
  MYFLT   *rcar, *rmod;
  MYFLT   *kfreq_max, *kfreq_min, *kband_max, *kband_min;
} DSH;



static int32_t dashow(CSOUND *csound, DSH *p)
{
  MYFLT range = *p->kband_max - *p->kband_min;
  if (range != FL(0.0))
    *p->rmod = (*p->kfreq_max - *p->kfreq_min) / range;
  else
    *p->rmod = FL(0.0);
  *p->rcar = (*p->kfreq_max - (*p->kband_max * *p->rmod));

  if (*p->rmod <= FL(0.0)) *p->rmod = FABS (*p->rmod);
  if (*p->rcar <= FL(0.0)) *p->rcar = FABS (*p->rcar);
  return OK;
}
#endif

/* -------------------------------------------------------------------- */


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "vtable1k",       S(MTABLE1),         TR,   "",  "kz",
                  (SUBR)mtable1_set,      (SUBR)mtable1_k,        (SUBR) NULL },
  { "trandom",        S(TRANGERAND),          0,      "k", "kkk",
                    NULL,                                   (SUBR)trRangeRand },
  { "lposcilsa", S(LPOSC_ST),  TR,  "aa","akkkio",
                             (SUBR)lposc_stereo_set, (SUBR)lposca_stereo},
  { "lposcilsa2", S(LPOSC_ST), TR,  "aa","akkkio",
                    (SUBR)lposc_stereo_set, (SUBR)lposca_stereo_no_trasp}
};

int32_t newgabopc_init_(CSOUND *csound) {
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

int32_t hvs_init_(CSOUND *csound);
int32_t slidertable_init_(CSOUND *csound);
int32_t tabmorph_init_(CSOUND *csound);
int32_t rbatonopc_init_(CSOUND *csound);


int32_t newgabopc_ModuleInit(CSOUND *csound)
{
  int32_t               err = 0;
  err |= hvs_init_(csound);
  err |= newgabopc_init_(csound);
  err |= tabmorph_init_(csound);
  /*      err |= rbatonopc_init_(csound); */


  return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}


#ifdef BUILD_PLUGINS
PUBLIC  int32_t     csoundModuleCreate(CSOUND *csound)
{
  return 0;
}

PUBLIC  int32_t     csoundModuleInit(CSOUND *csound)
{
  return  newgabopc_ModuleInit(csound);
}


PUBLIC int32_t csoundModuleInfo(void)
{
  return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}

PUBLIC  int32_t     csoundModuleDestroy(CSOUND *csound)
{
  return 0;
}

#endif

