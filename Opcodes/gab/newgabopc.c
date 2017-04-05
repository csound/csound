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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#include "csoundCore.h"
#include "interlocks.h"

/* (Shouldn't there be global decl's for these?) */
#define INCR (0.001f)

/* -------------------------------------------------------------------- */

typedef struct {
        OPDS    h;
        MYFLT    *xfn, *outargs[VARGMAX];
        int nargs;
        long  pfn;
        MYFLT   *ftable;
} MTABLE1;


static int  mtable1_set(CSOUND *csound, MTABLE1 *p)      /* mtab by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL)
      return csound->InitError(csound, Str("vtable1: incorrect table number"));
    p->ftable = ftp->ftable;
    p->nargs = p->INOCOUNT-1;
    p->pfn = (long) *p->xfn;
    return OK;
}

static int  mtable1_k(CSOUND *csound, MTABLE1 *p)
{
    int j, nargs = p->nargs;
    MYFLT **out = p->outargs;
    MYFLT *table;
    if (p->pfn != (long)*p->xfn) {
      FUNC *ftp;
      if ( (ftp = csound->FTFindP(csound, p->xfn) ) == NULL)
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("vtable1: incorrect table number"));
      p->pfn = (long)*p->xfn;
      p->ftable = ftp->ftable;
    }
    table= p->ftable;
    for (j=0; j < nargs; j++)
      *out[j] =  table[j];
    return OK;
}

/* -------------------------------------------------------------------- */

#if 0
static void unquote(char *dst, char *src)
{
    if (src[0] == '"') {
      int len = (int) strlen(src) - 2;
      strcpy(dst, src + 1);
      if (len >= 0 && dst[len] == '"')
        dst[len] = '\0';
    }
    else
      strcpy(dst, src);
}

PUBLIC int Sched(CSOUND *csound, MYFLT  *args[], int numargs) {
    double  starttime;
    int     i;
    EVTBLK  evt;
    char    name[512];

/*    if (p->XSTRCODE) {
             evt.strarg = (char*) p->args[0];
             evt.p[1] = SSTRCOD;
    }
    else */
    if (*args[1] == SSTRCOD) {
      unquote(name, csound->currevent->strarg);
      evt.strarg = name;
      evt.p[1] = SSTRCOD;
      evt.scnt = 1;
    }
    else {
      evt.strarg = NULL; evt.scnt = 0;
      evt.p[1] = *args[1];
    }
    evt.opcod = (char) *args[0];
    if (evt.opcod == 0) evt.opcod = 'i';

    evt.pcnt = numargs -1;
    /* Add current time (see note about kadjust in triginset() above) */
    starttime = *args[2];
    if(starttime < FL(0.0)) {
      starttime = FL(0.0);
    }
    starttime += (double) csound->GetKcounter(csound) /
                 (double) csound->GetKr(csound);
    /*starttime += (double) csound->global_kcounter / (double)csound->global_ekr;*/
   /* Copy all arguments to the new event */
    for (i = 0; i < numargs; i++)
      evt.p[i] = *args[i];

    if (evt.p[2] < FL(0.0)) {
      evt.p[2] = FL(0.0);
    }
    return (csound->insert_score_event(csound, &evt, starttime) == 0 ? OK : NOTOK);
}

/* These opcode has not been implemented because very similar ones exist */

typedef struct { /* GAB 11/Jan/2001 */
     OPDS   h;
     MYFLT  *trigger;
     MYFLT  *args[PMAX+1];
} SCHEDK;


static int schedk(CSOUND *csound, SCHEDK *p)
{ /* based on code by rasmus */
    if (*p->trigger) /* Only do something if triggered */
      Sched(csound, p->args, p->INOCOUNT-1);
    return OK;
}

static int schedk_i(CSOUND *csound, SCHEDK *p)
{ /* based on code by rasmus */
    OPARMS    *O = csound->oparms;

    O->RTevents = 1;     /* Make sure kperf() looks for RT events */
    /*   O->ksensing = 1; */
    /*   O->OrcEvts  = 1; */    /* - of the appropriate type */
    if (CS_KCNT > 0 && *p->trigger != FL(0.0) && p->h.insdshead->p3 == 0)
      schedk(csound,p);
    return OK;
}
/* -------------------------------------------------------------------- */

/* These opcodes have not been implemented */

typedef struct { /* GAB 11/Jan/2001 */
     OPDS   h;
     MYFLT  *trigger;
     MYFLT  *args[PMAX+1];
     MYFLT  args2[PMAX+1];
     MYFLT  *argums[PMAX+1];
     int nargs, done, doneRel;
     char *relesing;
     MYFLT frac;
} SCHEDINTIME;


static int schedInTime_set(CSOUND *csound, SCHEDINTIME *p)
{
     int *xtra;
     int j;
     OPARMS    *O = csound->oparms;
     /* hugly hack, needs to be inserted in the CSOUND structure */
     static MYFLT frac = 0;

     O->RTevents = 1;     /* Make sure kperf() looks for RT events */
     /*   O.ksensing = 1; */
     /*   O.OrcEvts  = 1; \/\* - of the appropriate type */
     p->frac = frac;
     p->nargs = p->INOCOUNT; /* num of arguments */
     for( j = 1; j < p->nargs; j++) {
       p->args2[j] = *p->args[j-1];
       p->argums[j]= &p->args2[j];
     }
     p->args2[0]=0;
     p->argums[0]=&p->args2[0];


     if (CS_KCNT > 0 && *p->trigger != 0 && p->h.insdshead->p3 == 0) {
       if (*p->argums[3] >= 0) *p->argums[3] = -1; /* start a held note */
       *p->argums[1] += p->frac;
       Sched(csound, p->argums, p->nargs);
       p->done = 1; /* only once */
     }
     else
       p->done = 0;
     p->doneRel = 0;
     xtra = &(p->h.insdshead->xtratim);
     if (*xtra < 1)
       *xtra = 1;
     p->relesing = &(p->h.insdshead->relesing);

     frac += INCR;
     frac = (frac >= 1) ? frac = 0 : frac;
     return OK;
}


static int schedInTime(CSOUND *csound, SCHEDINTIME *p)
{
    if (*p->trigger && !p->done) {/* Only do something if triggered */
      if (*p->argums[3] >= 0) *p->argums[3] = -1; /* start a held note */
      *p->argums[1] += p->frac;
      Sched(csound, p->argums, p->nargs);
      p->done = 1; /* only once */
    }
    if (*p->relesing && !p->doneRel) {
      *p->argums[1] = -*p->argums[1]; /* turn off the note with a negative p1 */
      Sched(csound, p->argums, p->nargs);
      p->doneRel = 1; /* only once */
    }
    return OK;
}


/* -------------------------------------------------------------------- */

/* These opocdes were not implemented because the functionality of      */
/* CopyTabElements has already been included in vcopy and vcopy_i       */

typedef struct { /* GAB 11/Jan/2001 */
     OPDS   h;
     MYFLT  *ktrigger, *idestTab, *kdestIndex, *isourceTab,
            *ksourceIndex, *inumElems;
     MYFLT *dTable, *sTable;
     long sLen, dLen;
} COPYTABELEMS;


static int copyTabElems_set(CSOUND *csound, COPYTABELEMS *p)
{
    FUNC *ftp;
    int nelems = (int) *p->inumElems;
    if ((ftp = csound->FTnp2Find(csound, p->idestTab)) == NULL)
      return csound->InitError(csound,
                               Str("copyTabElems: incorrect destination "
                                   "table number"));

    p->dLen = ftp->flen;
    if (nelems > p->dLen)
      return csound->InitError(csound,
                               Str("copyTabElems: destination table too short "
                                   "or number of elements to copy too big"));

    p->dTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->isourceTab)) == NULL)
      return csound->InitError(csound,
                               Str("copyTabElems: incorrect source table number"));

    p->sLen = ftp->flen;
    if (nelems > p->sLen)
      return csound->InitError(csound,
                               Str("copyTabElems: source table size less than "
                                   "the number of elements to copy"));

    p->sTable = ftp->ftable;

    return OK;
}


static int copyTabElems(CSOUND *csound, COPYTABELEMS *p)
{
    if(*p->ktrigger) {
      int nelems = (int) *p->inumElems;
      int j, sNdx = (int) *p->ksourceIndex * nelems,
             dNdx = (int) *p->kdestIndex * nelems;
      if (sNdx + nelems > p->sLen)
        return
          csound->PerfError(csound, p->h.insdshead,
                            Str("copyTabElems: source table too short"));
      if (dNdx + nelems > p->dLen)
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("copyTabElems: destination table too short"));

      for (j = 0; j< nelems; j++)
        p->dTable[dNdx+j] = p->sTable[sNdx+j];
    }
    return OK;
}

typedef struct { /* GAB 11/Jan/2001 */
     OPDS   h;
     MYFLT  *idestTab, *idestIndex, *isourceTab, *isourceIndex, *inumElems;
} COPYTABELEMS_I;

static int copyTabElemsi(CSOUND *csound, COPYTABELEMS_I *p)
{
    FUNC *ftp;
    int nelems = (int) *p->inumElems, dLen, sLen;
    MYFLT *sTable, *dTable;
    if ((ftp = csound->FTnp2Find(csound, p->idestTab)) == NULL)
      return csound->InitError(csound,
                               Str("copyTabElems: incorrect destination "
                                   "table number"));
    dLen = ftp->flen;
    if (nelems > dLen)
      return csound->InitError(csound,
                               Str("copyTabElems: destination table too short "
                                   "or number of elements to copy too big"));
    dTable = ftp->ftable;
    if ((ftp = csound->FTnp2Find(csound, p->isourceTab)) == NULL)
      return csound->InitError(csound,
                               Str("copyTabElems: incorrect source table number"));
    sLen = ftp->flen;
    if (nelems > sLen)
      return csound->InitError(csound,
                               Str("copyTabElems: source table size less than "
                                   "the number of elements to copy"));
    sTable = ftp->ftable;

    {
      int j, sNdx = (int) *p->isourceIndex * nelems,
        dNdx = (int) *p->idestIndex * nelems;
      if (sNdx + nelems > sLen)
        return
          csound->PerfError(csound, p->h.insdshead,
                            Str("copyTabElems: source table too short"));
      if (dNdx + nelems > dLen)
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("copyTabElems: destination table too short"));
      for (j = 0; j< nelems; j++) {
        dTable[dNdx+j] = sTable[sNdx+j];
      }
    }
    return OK;
}

#endif

/* -------------------------------------------------------------------- */

typedef struct {
        OPDS    h;
        MYFLT   *kstartChan, *argums[VARGMAX];
        int numChans, narg;
} INRANGE;

/* extern       MYFLT   *spin, *spout; */
/* extern    int spoutactive, PortaudioNumOfInPorts; gab default, = nchnls */

static int inRange_i(CSOUND *csound, INRANGE *p)
{
    p->narg = p->INOCOUNT-1;
/*p->numChans = (PortaudioNumOfInPorts == -1) ? nchnls : PortaudioNumOfInPorts; */
    if (!csound->oparms->sfread)
      return csound->InitError(csound, Str("inrg: audio input is not enabled"));
    p->numChans = csound->GetNchnls(csound);
    return OK;
}

static int inRange(CSOUND *csound, INRANGE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t j, nsmps = CS_KSMPS;
    int i;
    MYFLT *ara[VARGMAX];
    int startChan = (int) *p->kstartChan -1;
    MYFLT *sp = csound->spin + startChan;
    int narg = p->narg, numchans = p->numChans;

    if (startChan < 0)
      return csound->PerfError(csound, p->h.insdshead,
                               Str("inrg: channel number cannot be < 1 "
                                   "(1 is the first channel)"));

    if (UNLIKELY(early)) nsmps -= early;
    for (i = 0; i < narg; i++) {
      ara[i] = p->argums[i];
      if (UNLIKELY(offset)) memset(ara[i], '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) memset(&ara[i][nsmps], '\0', early*sizeof(MYFLT));
    }
    for (j=offset; j<nsmps; j++)  {
      for (i=0; i<narg; i++)
        ara[i][j] = sp[i];
      sp += numchans;
    }
    return OK;

}

#include "Opcodes/uggab.h"

static int lposc_set(CSOUND *csound, LPOSC *p)
{
    FUNC *ftp;
    MYFLT  loop, end, looplength;
    if ((ftp = csound->FTnp2Find(csound, p->ift)) == NULL)
      return csound->InitError(csound, Str("invalid function"));
    if (!(p->fsr=ftp->gen01args.sample_rate)){
       csound->Message(csound,
                       Str("lposc: no sample rate stored in function;"
                           " assuming=sr\n"));
       p->fsr=CS_ESR;
    }
    p->ftp    = ftp;
    p->tablen = ftp->flen;
    /* changed from
       p->phs    = *p->iphs * p->tablen;   */

    if ((loop = *p->kloop) < 0) loop=FL(0.0);
    if ((end = *p->kend) > p->tablen || end <=0 ) end = (MYFLT)p->tablen;
    looplength = end - loop;

    if (*p->iphs >= 0)
      p->phs = *p->iphs;
    while (p->phs >= end)
      p->phs -= looplength;
    return OK;
}

static int lposca(CSOUND *csound, LPOSC *p)
{
    double  *phs= &p->phs;
    double  si= *p->freq * (p->fsr/CS_ESR);
    MYFLT   *out = p->out,  *amp=p->amp;
    MYFLT   *ft =  p->ftp->ftable, *curr_samp;
    MYFLT   fract;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32   loop, end, looplength /* = p->looplength */ ;

    if ((loop = (long) *p->kloop) < 0) loop=0;/* gab */
    else if (loop > p->tablen-3) loop = p->tablen-3;
    if ((end = (long) *p->kend) > p->tablen-1 ) end = p->tablen - 1;
    else if (end <= 2) end = 2;
    if (end < loop+2) end = loop + 2;
    looplength = end - loop;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      curr_samp= ft + (long)*phs;
      fract= (MYFLT)(*phs - (long)*phs);
      out[n] = amp[n] * (*curr_samp +(*(curr_samp+1)-*curr_samp)*fract);
      *phs += si;
      while (*phs  >= end) *phs -= looplength;
      while (*phs  < loop) *phs += looplength;
    }
    return OK;
}

/* -------------------------------------------------------------------- */

typedef struct  {
        OPDS    h;
        MYFLT   *out1, *out2, *amp, *freq, *kloop, *kend, *ift, *iphs;
        long    tablen;
        MYFLT   fsr;
        MYFLT *ft; /*table */
        double  phs, fsrUPsr /* , looplength */;
        long    phs_int;
} LPOSC_ST;

static int lposc_stereo_set(CSOUND *csound, LPOSC_ST *p)
{
    FUNC *ftp;
    double  loop, end, looplength, fsr;
    if ((ftp = csound->FTnp2Find(csound, p->ift)) == NULL)
      return csound->InitError(csound, Str("invalid function"));
    if (!(fsr = ftp->gen01args.sample_rate)) {
      csound->Message(csound, Str("lposcil: no sample rate stored in function;"
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
      p->phs_int = (long) (p->phs = *p->iphs);

    while (p->phs >= end)
      p->phs_int = (long) (p->phs -= looplength);
    return OK;
}

static int lposca_stereo(CSOUND *csound, LPOSC_ST *p) /* stereo lposcinta */
{
    double  *phs= &p->phs,   si= *p->freq * p->fsrUPsr;
    MYFLT   *out1 = p->out1, *out2 = p->out2, *amp=p->amp;
    MYFLT   *ft =  p->ft;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32    loop, end, looplength /* = p->looplength */ ;
    if ((loop = (long) *p->kloop) < 0) loop=0;/* gab */
    else if (loop > p->tablen-3) loop = p->tablen-3;
    if ((end = (long) *p->kend) > p->tablen-1 ) end = p->tablen - 1;
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
      MYFLT *curr_samp1 = ft + (long) *phs * 2;
      MYFLT *curr_samp2 = curr_samp1 +1;
      fract= *phs - (long) *phs;
      out1[n] = amp[n] * (MYFLT)(*curr_samp1 +(*(curr_samp1+2)-*curr_samp1)*fract);
      out2[n] = amp[n] * (MYFLT)(*curr_samp2 +(*(curr_samp2+2)-*curr_samp2)*fract);
      *phs += si;
      while (*phs  >= end) *phs -= looplength;
      while (*phs  < loop) *phs += looplength;
    }
    return OK;
}

static int lposca_stereo_no_trasp(CSOUND *csound, LPOSC_ST *p)
{    /* transposition is allowed only */
     /*in integer values (twice, three times etc.) so it is faster */
    long    *phs = &p->phs_int, si = (long) *p->freq;
    MYFLT   *out1 = p->out1, *out2 = p->out2, *amp=p->amp;
    MYFLT   *ft =  p->ft;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    long    loop, end, looplength /* = p->looplength */ ;
    if ((loop = (long) *p->kloop) < 0) loop=0;/* gab */
    else if (loop > p->tablen-3) loop = p->tablen-3;
    if ((end = (long) *p->kend) > p->tablen-1 ) end = p->tablen - 1;
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

static int trRangeRand(CSOUND *csound, TRANGERAND *p)
{ /* gab d5*/
    if (*p->ktrig)
      *p->out = p->lastvalue = randGab * (*p->max - *p->min) + *p->min;
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



static int dashow (CSOUND *csound, DSH *p)
{
    MYFLT range = *p->kband_max - *p->kband_min;
    if (range != FL(0.0))
      *p->rmod = (*p->kfreq_max - *p->kfreq_min) / (*p->kband_max - *p->kband_min);
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
  { "vtable1k",       S(MTABLE1),         TR, 3,  "",  "kz",
                  (SUBR)mtable1_set,      (SUBR)mtable1_k,        (SUBR) NULL },
  /*      { "schedk",         S(SCHEDK),     0,     3,  "",  "kkz",
                  (SUBR)schedk_i,         (SUBR) schedk,          (SUBR) NULL }, */
  /*      { "schedInTime",    S(SCHEDINTIME), 0,    3,  "",  "kz",
                  (SUBR)schedInTime_set,  (SUBR)schedInTime ,     (SUBR) NULL }, */
  /*      { "copyTabElems",   S(COPYTABELEMS),0,    3,  "",  "kikiki",
                  (SUBR)copyTabElems_set, (SUBR)copyTabElems,     (SUBR)NULL  }, */
  /*      { "copyTabElemsi",  S(COPYTABELEMS_I), 0, 1,  "",  "iiiii",
                  (SUBR)copyTabElemsi,    (SUBR)NULL,             (SUBR)NULL  }, */
  /*      { "Trandom",        S(TRANGERAND),        0,      2,  "k", "kkk",
                    NULL,                                   (SUBR)trRangeRand }, */
  { "trandom",        S(TRANGERAND),          0,    2,  "k", "kkk",
                    NULL,                                   (SUBR)trRangeRand },
  /*      { "lposcinta",      S(LPOSC),        0,   5,      "a", "akkkio",
                                      (SUBR)lposc_set,  NULL,  (SUBR)lposcinta}, */
  /*      { "lposcintsa",     S(LPOSCINT_ST),  0,   5,  "aa","akkkio",
                        (SUBR)lposcint_stereo_set,NULL, (SUBR)lposcinta_stereo}, */
  /*      { "lposcintsa2",    S(LPOSCINT_ST),   0,  5,  "aa","akkkio",
              (SUBR)lposcint_stereo_set,NULL,  (SUBR)lposcinta_stereo_no_trasp}, */
  { "lposcila", S(LPOSC),      TR, 5, "a", "akkkio",
                                           (SUBR)lposc_set, NULL, (SUBR)lposca},
  { "lposcilsa", S(LPOSC_ST),  TR, 5, "aa","akkkio",
                             (SUBR)lposc_stereo_set, NULL, (SUBR)lposca_stereo},
  { "lposcilsa2", S(LPOSC_ST), TR, 5, "aa","akkkio",
                    (SUBR)lposc_stereo_set, NULL, (SUBR)lposca_stereo_no_trasp},
  /* { "dashow.i", S(DSH), 0,1,  "ii","iiii", (SUBR)dashow     }, */
  /* { "dashow.k", S(DSH), 0,2,  "kk","kkkk", NULL, (SUBR)dashow   }, */
  { "inrg", S(INRANGE), 0,5, "", "ky", (SUBR)inRange_i, (SUBR)NULL, (SUBR)inRange }


};

int newgabopc_init_(CSOUND *csound) {
   return csound->AppendOpcodes(csound, &(localops[0]),
                                (int) (sizeof(localops) / sizeof(OENTRY)));
}

int hvs_init_(CSOUND *csound);
int slidertable_init_(CSOUND *csound);
int tabmorph_init_(CSOUND *csound);
int rbatonopc_init_(CSOUND *csound);


int newgabopc_ModuleInit(CSOUND *csound)
{
        int               err = 0;
        err |= hvs_init_(csound);
        err |= newgabopc_init_(csound);
        err |= slidertable_init_(csound);
        err |= tabmorph_init_(csound);
        /*      err |= rbatonopc_init_(csound); */


        return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

/*
PUBLIC  int     csoundModuleDestroy(CSOUND *csound)
{
    return 0;
}
*/
