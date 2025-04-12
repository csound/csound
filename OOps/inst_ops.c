/*
  inst_ops.c: instrument and instance opcodes

  Copyright (C) 2025 Victor Lazzarini

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


#include "csoundCore.h" 
#include "inst_ops.h"     /* for goto's */
#include "namedins.h"   
#include "interlocks.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include <inttypes.h>

int32_t kill_instance(CSOUND *csound, KILLOP *p) {
  INSDS *insds;
  // pick up instance from var memory
  memcpy(&insds, p->inst, sizeof(INSDS *));
  if (LIKELY(insds))  {
    if(insds->actflg == 0) return OK;
    xturnoff(csound, insds);
  }
  else csound->Warning(csound, Str("instance not valid\n"));
  return OK;
}


int32_t kill_instancek(CSOUND *csound, KILLOP *p) {
  int32_t res = OK;
  if((int32_t) *p->ktrig != 0)
    res = kill_instance(csound, p);
  if (!p->h.insdshead->actflg) {  /* if current note was deactivated: */
    while (CS_PDS->nxtp != NULL)
      CS_PDS = CS_PDS->nxtp;    /* loop to last opds */
  }
  return res;
}

int32_t nstrnumset(CSOUND *csound, NSTRNUM *p)
{
  /* IV - Oct 31 2002 */
  int32_t res = string_arg_to_insno(csound, p->iname, 0);
  if (UNLIKELY(res == NOT_AN_INSTRUMENT)) {
    *p->i_insno = -FL(1.0); return NOTOK;
  }
  else {
    *p->i_insno = (MYFLT)res; return OK;
  }
}

int32_t nstrnumset_S(CSOUND *csound, NSTRNUM *p)
{
  /* IV - Oct 31 2002 */
  int32_t res = string_arg_to_insno(csound, ((STRINGDAT *)p->iname)->data, 1);
  if (UNLIKELY(res == NOT_AN_INSTRUMENT)) {
    *p->i_insno = -FL(1.0); return NOTOK;
  }
  else {
    *p->i_insno = (MYFLT)res; return OK;
  }
}

int32_t nstrstr(CSOUND *csound, NSTRSTR *p)
{
  char *ss;
  if (csound->engineState.instrumentNames) {
    ss = cs_inverse_hash_get(csound,
                             csound->engineState.instrumentNames,
                             (int)*p->num);
  }
  else ss= "";
  mfree(csound,p->ans->data);
  p->ans->data = cs_strdup(csound, ss);
  p->ans->size = strlen(ss);
  return OK;
}

int32_t prealloc_(CSOUND *csound, AOP *p, int32_t instname)
{
  int32_t     n, a;

  if (instname)
    n = (int32_t) string_arg_to_opcno(csound, ((STRINGDAT*)p->r)->data, 1,
                               (*p->b == FL(0.0) ? 0 : 1));
  else {
    if (IsStringCode(*p->r))
      n = (int32_t) string_arg_to_opcno(csound, get_arg_string(csound,*p->r), 1,
                                 (*p->b == FL(0.0) ? 0 : 1));
    else n = *p->r;
  }
  if (UNLIKELY(n == NOT_AN_INSTRUMENT)) return NOTOK;
  if (csound->oparms->realtime)
    csoundSpinLock(&csound->alloc_spinlock);
  a = (int32_t) *p->a - csound->engineState.instrtxtp[n]->active;
  for ( ; a > 0; a--)
    instance(csound, n);
  if (csound->oparms->realtime)
    csoundSpinUnLock(&csound->alloc_spinlock);
  return OK;
}

int32_t prealloc(CSOUND *csound, AOP *p){
  return prealloc_(csound,p,0);
}

int32_t prealloc_S(CSOUND *csound, AOP *p){
  return prealloc_(csound,p,1);
}

int32_t delete_instr(CSOUND *csound, DELETEIN *p)
{
  int32_t       n;
  INSTRTXT  *ip;
  INSDS     *active;
  INSTRTXT  *txtp;

  if (IS_STR_ARG(p->insno))
    n = csound->StringArg2Insno(csound, ((STRINGDAT *)p->insno)->data, 1);
  else if (GetTypeForArg(p->insno) == &CS_VAR_TYPE_INSTR) {
    n = instr_num(csound, ((INSTREF *)p->insno)->instr);
  }
  else
    n = (int32_t) (*p->insno + FL(0.5));

  if (UNLIKELY(n == NOT_AN_INSTRUMENT ||
               n > csound->engineState.maxinsno ||
               csound->engineState.instrtxtp[n] == NULL))
    return OK;                /* Instrument does not exist so noop */
  ip = csound->engineState.instrtxtp[n];
  active = ip->instance;
  while (active != NULL) {    /* Check there are no active instances */
    INSDS   *nxt = active->nxtinstance;
    if (UNLIKELY(active->actflg)) { /* Can only remove non-active instruments */
      char *name = csound->engineState.instrtxtp[n]->insname;
      if (name)
        return csound->InitError(csound,
                                 Str("Instrument %s is still active"), name);
      else
        return csound->InitError(csound,
                                 Str("Instrument %d is still active"), n);
    }
#if 0
    if (active->opcod_iobufs && active->insno > csound->engineState.maxinsno)
      csound->Free(csound, active->opcod_iobufs);        /* IV - Nov 10 2002 */
#endif
    if (active->fdchp != NULL)
      fdchclose(csound, active);
    if (active->auxchp != NULL)
      auxchfree(csound, active);
    free_instr_var_memory(csound, active);
    csound->Free(csound, active);
    active = nxt;
  }
  csound->engineState.instrtxtp[n] = NULL;
  /* Now patch it out */
  for (txtp = &(csound->engineState.instxtanchor);
       txtp != NULL;
       txtp = txtp->nxtinstxt)
    if (txtp->nxtinstxt == ip) {
      OPTXT *t = ip->nxtop;
      txtp->nxtinstxt = ip->nxtinstxt;
      while (t) {
        OPTXT *s = t->nxtop;
        csound->Free(csound, t);
        t = s;
      }
      csound->Free(csound, ip);
      return OK;
    }
  return NOTOK;
}

/** offsetsmps
 *  returns the sample accurate offset at i or k time
 *
 *  [i/k]offs offsetsmps
*/    
int32 sa_offset(CSOUND *csound, AOP *p){
  *p->r = p->h.insdshead->ksmps_offset;
  return OK;
}

/** earlysmps
 *  returns the sample accurate early exit length at k time
 *
 *  kearly earlysmps
*/    
int32 sa_early(CSOUND *csound, AOP *p){
  *p->r = p->h.insdshead->ksmps_no_end;
  return OK;
}

/* performs one k-cycle for instance *ip
   - should only be called at perf time
   - should only be called on instances not in activ chain,
   that is, created by init_instance
   - checks for actflg so it's compatible with pause
  
   Returns CSOUND_SUCCESS or an error code.
*/
static int32_t perf_instance(CSOUND *csound, INSDS *ip) {
  int32_t error = CSOUND_SUCCESS;
  OPDS  *opstart = (OPDS*) ip;
  ip->spin = csound->spin;
  ip->spout = csound->spout_tmp;
  ip->kcounter =  csound->kcounter;
  if (ip->ksmps == csound->ksmps) {
    csound->mode = 2;
    while (error == 0 && opstart != NULL &&
           (opstart = opstart->nxtp) != NULL && ip->actflg) {
      opstart->insdshead->pds = opstart;
      csound->op = opstart->optext->t.opcod;
      error = (*opstart->perf)(csound, opstart); /* run each opcode */
      opstart = opstart->insdshead->pds;
    }
    csound->mode = 0;
  } else {
    int32_t i, n = csound->nspout, start = 0;
    int32_t lksmps = ip->ksmps;
    int32_t incr = csound->nchnls*lksmps;
    ip->kcounter = (csound->kcounter-1)*csound->ksmps/lksmps;
    for (i=start; i < n; i+=incr, ip->spin+=incr, ip->spout+=incr) {
      ip->kcounter++;
      opstart = (OPDS*) ip;
      csound->mode = 2;
      while (error ==  0 && (opstart = opstart->nxtp) != NULL &&
             ip->actflg) {
        opstart->insdshead->pds = opstart;
        csound->op = opstart->optext->t.opcod;
        error = (*opstart->perf)(csound, opstart); /* run each opcode */
        opstart = opstart->insdshead->pds;
      }
      csound->mode = 0;
    }
  }
  return error;
}

/* Splice instance ip before ipnxt
   returns CSOUND_SUCCESS or CSOUND_ERROR
*/
static int32_t splice_before_instance(CSOUND *csound,
                                      INSDS *ip, INSDS *ipnxt){
  INSDS *nxtp, *prvp;
  if(ip == NULL) return CSOUND_ERROR;
  if(ip->prvact != NULL) {
    // unlink first
    ip->prvact->nxtact = ip->nxtact;
  }
  nxtp = &(csound->actanchor);    
  while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
    if (nxtp == ipnxt) {
      nxtp->prvact = ip;
      break;
    }
  }
  if(nxtp == NULL) return CSOUND_ERROR;
  ip->nxtact = nxtp;
  ip->prvact = prvp;
  prvp->nxtact = ip;
  return CSOUND_SUCCESS;
}

/* Splice instance ip after ipprev
   returns CSOUND_SUCCESS or CSOUND_ERROR
*/
static int32_t splice_after_instance(CSOUND *csound,
                                  INSDS *ip, INSDS *ipprev){
  INSDS *nxtp;
  if(ip != NULL) return CSOUND_ERROR;
  if(ip->prvact != NULL) {
    // unlink first
    ip->prvact->nxtact = ip->nxtact;
  }
  nxtp = &(csound->actanchor);    
  while (nxtp != NULL) {
    if (nxtp == ipprev) {
      ip->nxtact = nxtp->nxtact;
      nxtp->nxtact = ip;
      break;
    }
    nxtp = nxtp->nxtact;
  }
  if(nxtp == NULL) return CSOUND_ERROR;
  ip->prvact = nxtp;
  return CSOUND_SUCCESS;
}


// Instance manipulation opcodes
#include "linevent.h"
/* play opcode
   plays an instrument given as an instrument definition reference 
   indefinitely and returns an instance ref

   var:Instr play InstrRef[, p4, ...] 

   runs only at i-time

   NB: instance is added to the end of activ chain regardless of instr number.
   Instance is NOT added to instr act_instance chain and can be deleted.
*/
int32_t play_instr(CSOUND *csound, LINEVENT2 *p) {
  if(p->inst->readonly)
    return
      csound->InitError(csound, "cannot write to instance ref for instr %d\n"
                        "- read-only variable",
                        instr_num(csound, ((INSTREF *) p->args[0])->instr));
  else {
    EVTBLK  evt;
    INSDS *ip;
    int32_t res, i;
    INSTREF *ref = (INSTREF *) p->args[0];
    res = instr_num(csound, ref->instr);
    evt.strarg = NULL; evt.scnt = 0;
    evt.opcod = 'i';
    evt.pcnt = p->INOCOUNT + 2;
    evt.p[1] = FL(res);
    evt.p[2] = FL(0.0);
    evt.p[3] = -1;
    for (i = 4; i <= evt.pcnt; i++)
      evt.p[i] = *p->args[i-3];
  
    ip = create_instance(csound, res);
    if(ip != NULL) {
      int32_t err = init_instance(csound, ip, &evt);
      if(err == 0) {
        INSDS *prvp, *nxtp;
        nxtp = &(csound->actanchor);   
        // splice at end of chain
        while ((prvp = nxtp) &&
               (nxtp = prvp->nxtact) != NULL)
          ;
        ip->nxtact = nxtp;
        ip->prvact = prvp;
        prvp->nxtact = ip;
        ((INSTANCEREF *) p->inst)->instance = ip;
        const OPARMS* O = csound->GetOParms(csound);
        if (UNLIKELY(O->msglevel & CS_RNGEMSG)) {
          char *name = csound->engineState.instrtxtp[ip->insno]->insname;
          if(csound->GetDebug(csound)) {
            if (UNLIKELY(name))
              csound->ErrorMsg(csound,
                               Str("instance %llu (instr %s): "
                                   "linked and active\n"),
                               ip->instance_id, name);
            else
              csound->ErrorMsg(csound,
                               Str("instance %llu (instr %d): "
                                   "linked and active\n"),
                               ip->instance_id, ip->insno);
          }
        }
        return OK;
      }
      else return csound->InitError(csound,
                                    "could not initialise instr %d",
                                    res);
    } else return csound->InitError(csound,
                                    "could not instantiate instr %d",
                                    res);
  }
}

/** Instance create opcode
    creates a read-only instance ref of instr ref

    var:Instr  init  InstrRef

    runs only i-time
*/
int32_t create_instance_opcode(CSOUND *csound, CREATE_INSTANCE *p) {
  if(p->out->readonly) return
                         csound->InitError(csound, "cannot write to instance ref for instr %d\n"
                                           "- read-only variable",
                                           instr_num(csound,p->in->instr));
  INSDS *ip = create_instance(csound, instr_num(csound,p->in->instr));
  if(ip != NULL) {
    p->out->instance = ip;
    return OK;
  } else return csound->InitError(csound,
                                  "could not instantiate instr %d",
                                  instr_num(csound,p->in->instr));  
}

/** Instance init opcode
    runs init-pass on instr instance ref

    error:i  init  Instr[,p4, ...]

    runs only at i-time
*/
int32_t init_instance_opcode(CSOUND *csound, INIT_INSTANCE *p) {
  EVTBLK evt;
  INSTANCEREF *ref = (INSTANCEREF *) p->args[0];
  int32_t i;
  if(ref->instance != NULL) {
    evt.p[1] = FL(ref->instance->insno);
    evt.p[2] = FL(0.0);
    evt.p[3] = -1;
    evt.strarg = NULL;
    evt.scnt = 0;
    evt.opcod = 'i';
    evt.pcnt = p->INOCOUNT + 2;
    for (i = 4; i <= evt.pcnt; i++)
      evt.p[i] = *p->args[i-3];
    // pass on any init errors to output, do not act on them
    *p->err = init_instance(csound, ref->instance, &evt);
    return OK;
  } else 
    return csound->InitError(csound, "NULL instance\n");
}

/** Instance performance opcode
    single perf-pass for instance

    err:k perf Instr

    runs only at perf-time
*/
int32_t perf_instance_opcode(CSOUND *csound, PERF_INSTR *p) {
  INSDS *ip = p->in->instance; 
  if(ip != NULL) {
    if(instr_context_check(csound, ip, p->h.insdshead) == OK){
    // check for initialisation flag, pass on any perf errors
    if (ip->init_done) 
      *p->out = FL(perf_instance(csound, ip));
    else return csound->PerfError(csound, &(p->h),  
                                  "instr %d not initialised\n",
                                  ip->insno);
    } else {
      return csound->PerfError(csound, &(p->h), "context mismatch, "
                               "cannot perform instr %d instance",
                      ip->insno);
    }
  }
  else csound->PerfError(csound, &(p->h),  
                         "NULL instance\n");
  return OK;
}
/** Instance deletion
    turns off and deletes instance (if possible)

    delete Instr

    runs only at deinit time
*/
int32_t delete_instance_opcode(CSOUND *csound, DEL_INSTR *p) {
  INSDS *ip = p->in->instance;
  if(ip != NULL) free_instance(csound, ip);
  p->in->instance = NULL;
  p->in->readonly = 0;
  return OK;
}

/** Instance performance pause
    pauses/unpauses instrument according to off 

    pause Instr, off:k

    runs only at perf-time
*/
int32_t pause_instance_opcode(CSOUND *csound, PAUSE_INSTR *p) {
  INSDS *ip = p->in->instance;
  if(ip != NULL)
    ip->actflg = *p->pause == 0 ? 1 : 0;
  return OK;
}

/** Instance parameter setting
    set instance pfield at perf-time

    setp Instr, pfield:k, val:k

    runs only at perf-time
*/
int32_t set_instance_parameter(CSOUND *csound, PARM_INSTR *p){
  INSDS *ip = p->in->instance;
  if(ip != NULL) {
    int32_t n = *p->par;
    INSTRTXT *tp = csound->engineState.instrtxtp[ip->insno];
    if(n <= tp->pmax) {
      CS_VAR_MEM* pfield = ((CS_VAR_MEM*) &ip->p0) + n + 1;
      pfield->value = *p->val;
    }
  }
  return OK;
}

/** Instance reference retrieval
    get instance of current instr

    var:Instr getinstance  

    runs only at i-time
*/
int32_t get_instance(CSOUND *csound, DEL_INSTR *p) {
  p->in->instance = p->h.insdshead;
  return OK;
}

/** Instance order manipulation
    splice instance before or after another instance

    err:i splice Instr1, Instr2, imode 

    runs only at i-time
*/
int32_t splice_instance(CSOUND *csound, SPLICE_INSTR *p) {
  if(p->in->instance && p->nxt->instance)
    *p->out = (MYFLT) ((int32_t) *p->mode == 0 ?
      splice_before_instance(csound, p->in->instance,
                             p->nxt->instance) :
      splice_after_instance(csound, p->in->instance,
                            p->nxt->instance));
  else *p->out = FL(-1.);
  return OK;
}



MYFLT initialise_io(CSOUND *csound);
/** experimental perf loop opcode to run on instr 0
    -- not for production --
    OENTRY entry = 
    { "perfloop", S(PERF_INSTR), 0,  "", "i:Instr;", 
    (SUBR) perf_loop_opcode, NULL, NULL };
*/
int32_t perf_loop_opcode(CSOUND *csound, PERF_INSTR *p) {
  INSDS *ip = p->in->instance;
  int32_t err = 0;
  size_t count = (size_t) (*p->out*csoundGetKr(csound));
  if(p->h.insdshead->insno == 0) {
    if(csound->spoutran == NULL)
      initialise_io(csound);
    while(ip != NULL && --count && !err) {
      memset(csound->spout, 0, csound->nspout*sizeof(MYFLT));
      memset(csound->spout_tmp,0,
             sizeof(MYFLT)*csound->nspout*csound->oparms->numThreads);
      err = perf_instance(csound, ip);
      csound->spoutran(csound); /* send to audio_out */
    }
  } else csound->Warning(csound, "perfloop: only allowed in instr 0\n");
  return OK;
}
