/*
  udo.c: user-defined opcodes and subinstruments

  Copyright (C) 2003-2025 Steven Yi, Victor Lazzarini, Istvan Varga,
                          Matt Ingalls

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

#include "udo.h"
#include "Opcodes/biquad.h"
#include "csound_data_structures.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "namedins.h"

/* Sets up pass-by-ref for input/output to/from UDO instance.
* Will search for xin/xout opcodes in init chain to read variable to
* setup VARPOOL, iterate init and perf chains to do arg lookup of each
* opcode's args and set pointers from parent instrument.
*
* 1. Iterate through the init chain to find xin/xout opcodes.
* 2. When found, lookup names of input and output variables for this current UDO instance.
* 3. Setup a map of input/output variable names to passed in pointers
* 4. Iterate through the init chain to find references to xin/xout vars and set
* the pointers to the input/out variable argument pointer addresses.
* 5. Iterate through the perf chain to find references to xin/xout vars and set
* the pointers to the input/out variable argument pointer addresses.
*/
static void handle_pass_by_ref(CSOUND* csound, UOPCODE* p, INSDS* lcurip) {
  /* NEW CODE FOR SETTING REFERENCES */

  // csound->Message(csound, "Pass-by-ref UDO %s\n", p->h.optext->t.oentry->opname);

  int32_t  i;
  OPDS *ichain = lcurip->nxti;
  OPDS *pchain = lcurip->nxtp;

  CS_HASH_TABLE *arg_ptr_map = cs_hash_table_create(csound);

  // Search xin/xout to setup arg_ptr_map
  while (ichain != NULL) {
    OPTXT *optext = ichain->optext;
    // printf("ichain: %s\n", optext->t.opcod);

    if (strcmp("xin", optext->t.opcod) == 0) {
      ARGLST *outlist = optext->t.outlist;
    //   printf("xin found\n");

      // MAP input args for this UDO to outputs of xin for the UDO
      for (i = 0; i < outlist->count; i++) {
        char *varName = outlist->arg[i];
        // printf("ar index %d\n", p->OUTOCOUNT + i);
        MYFLT *argPtr = p->ar[p->OUTOCOUNT + i];

        // printf("Storing arg %s to %p\n", varName, (void*)argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);
        cs_hash_table_put(csound, arg_ptr_map, varName, argPtr);
      }
    } else if (strcmp("xout", ichain->optext->t.opcod) == 0) {
      ARGLST *inlist = optext->t.inlist;
    //   printf("xout found\n");
      // MAP output args for this UDO to inputs of xout for the UDO
      for (i = 0; i < inlist->count; i++) {
        char *varName = inlist->arg[i];

        // printf("ar index %d\n", i);
        MYFLT *argPtr = p->ar[i];

        // printf("Storing arg %s to %p\n", varName, (void*)argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);
        cs_hash_table_put(csound, arg_ptr_map, varName, argPtr);
      }
    }

    ichain = ichain->nxti;
  }

  ichain = lcurip->nxti;

  while (ichain != NULL) {
    OPTXT *optext = ichain->optext;
    ARGLST *outlist = optext->t.outlist;
    ARGLST *inlist = optext->t.inlist;
    bool isUdo = optext->t.oentry->useropinfo != NULL;

    for (i = 0; i < outlist->count; i++) {
      char *varName = outlist->arg[i];
      MYFLT *argPtr = (MYFLT *)cs_hash_table_get(csound, arg_ptr_map, varName);
      if (argPtr != NULL) {
        // printf("Setting arg %s to %p\n", varName, argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);
        if(isUdo) {
            UOPCODE *udoData = (UOPCODE *)ichain;
            udoData->ar[i] = argPtr;
        } else {
            MYFLT** argStart = (MYFLT**)(ichain + 1);
            argStart[i] = argPtr;
        }
      }
    }

    for (i = 0; i < inlist->count; i++) {
      char *varName = inlist->arg[i];
      MYFLT *argPtr = (MYFLT *)cs_hash_table_get(csound, arg_ptr_map, varName);
      if (argPtr != NULL) {
        // printf("Setting arg %s to %p\n", varName, argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);

        if(isUdo) {
            UOPCODE *udoData = (UOPCODE *)ichain;
            udoData->ar[outlist->count + i] = argPtr;
        } else {
            MYFLT** argStart = (MYFLT**)(ichain + 1);
            argStart[outlist->count + i] = argPtr;
        }
      }
    }

    ichain = ichain->nxti;
  }

  while (pchain != NULL) {
    // printf("pchain: %s\n", pchain->optext->t.opcod);

    OPTXT *optext = pchain->optext;
    ARGLST *outlist = optext->t.outlist;
    ARGLST *inlist = optext->t.inlist;
    bool isUdo = optext->t.oentry->useropinfo != NULL;

    for (i = 0; i < outlist->count; i++) {
      char *varName = outlist->arg[i];
      MYFLT *argPtr = (MYFLT *)cs_hash_table_get(csound, arg_ptr_map, varName);
      if (argPtr != NULL) {
        // printf("Setting arg %s to %p\n", varName, argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);
        if(isUdo) {
            UOPCODE *udoData = (UOPCODE *)pchain;
            udoData->ar[i] = argPtr;
        } else {
            MYFLT** argStart = (MYFLT**)(pchain + 1);
            argStart[i] = argPtr;
        }
      }
    }

    for (i = 0; i < inlist->count; i++) {
      char *varName = inlist->arg[i];
      MYFLT *argPtr = (MYFLT *)cs_hash_table_get(csound, arg_ptr_map, varName);
      if (argPtr != NULL) {
        // printf("Setting arg %s to %p\n", varName, argPtr);
        // printf("Cur value: %g\n", *(MYFLT*)argPtr);
        if(isUdo) {
            UOPCODE *udoData = (UOPCODE *)pchain;
            udoData->ar[outlist->count + i] = argPtr;
        } else {
            MYFLT** argStart = (MYFLT**)(pchain + 1);
            argStart[outlist->count + i] = argPtr;
        }
      }
    }
    pchain = pchain->nxtp;
  }

  cs_hash_table_free(csound, arg_ptr_map);

  /* END NEW CODE FOR SETTING REFERENCES */

}

/*
  UDOs now use the local ksmps/local sr stored in lcurip, and
  all the other dependent parameters are calculated in relation to
  this.

  lcurip->ksmps is set to the caller ksmps (CS_KSMPS), unless a new
  local ksmps is used, in which case it is set to that value.
  Local ksmps can only be set by setksmps.
  If local ksmps differs from CS_KSMPS, we set useropcd1() to
  deal with the perf-time code. Otherwise useropcd2() is used.

  For recursive calls when the local ksmps is set to differ from
  the calling instrument ksmps, the top-level call
  will use useropcd1(), whereas all the other recursive calls
  will use useropdc2(), since their local ksmps will be the same
  as the caller.

  Also in case of a local ksmps that differs from the caller,
  the local kcounter value, obtained from the caller is
  scaled to denote the correct kcount in terms of local
  kcycles.

  Similarly, a local SR is now implemented. This is set by
  the oversample/undersample opcode. It is not allowed with
  local ksmps setting (setksmps) or with audio/k-rate array
  arguments. It uses useropcd2().

*/

int32_t useropcdset(CSOUND *csound, UOPCODE *p)
{
  OPDS         *saved_ids = csound->ids;
  INSDS        *parent_ip = csound->curip, *lcurip;
  INSTRTXT     *tp;
  uint32_t instno;
  uint32_t i;
  OPCODINFO    *inm;
  OPCOD_IOBUFS *buf = p->buf;

  /* look up the 'fake' instr number, and opcode name */
  inm = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
  instno = inm->instno;
  tp = csound->engineState.instrtxtp[instno];
  if (tp == NULL)
    return csound->InitError(csound, Str("Cannot find instr %d (UDO %s)\n"),
                             instno, inm->name);
  if (!p->ip) {
    /* search for already allocated, but not active instance */
    /* if none was found, allocate a new instance */
    tp = csound->engineState.instrtxtp[instno];
    if (tp == NULL) {
      return csound->InitError(csound, Str("Cannot find instr %d (UDO %s)\n"),
                               instno, inm->name);
    }
    if (!tp->act_instance)
      instance(csound, instno);
    lcurip = tp->act_instance;            /* use free instance, and */
    tp->act_instance = lcurip->nxtact;    /* remove from chain      */
    if (lcurip->opcod_iobufs==NULL)
      return csound->InitError(csound, "Broken redefinition of UDO %d (UDO %s)\n",
                               instno, inm->name);
    lcurip->actflg++;                     /*    and mark the instr active */
    tp->active++;
    tp->instcnt++;
    /* link into deact chain */
    lcurip->opcod_deact = parent_ip->opcod_deact;
    lcurip->subins_deact = NULL;
    parent_ip->opcod_deact = (void*) p;
    p->ip = lcurip;
    /* IV - Nov 10 2002: set up pointers to I/O buffers */
    buf = p->buf = (OPCOD_IOBUFS*) lcurip->opcod_iobufs;
    buf->opcode_info = inm;
    /* initialise perf time address lists */
    /* **** Could be a memset **** */
    buf->iobufp_ptrs[0] = buf->iobufp_ptrs[1] = NULL;
    buf->iobufp_ptrs[2] = buf->iobufp_ptrs[3] = NULL;
    buf->iobufp_ptrs[4] = buf->iobufp_ptrs[5] = NULL;
    buf->iobufp_ptrs[6] = buf->iobufp_ptrs[7] = NULL;
    buf->iobufp_ptrs[8] = buf->iobufp_ptrs[9] = NULL;
    buf->iobufp_ptrs[10] = buf->iobufp_ptrs[11] = NULL;
    /* store parameters of input and output channels, and parent ip */
    buf->uopcode_struct = (void*) p;
    buf->parent_ip = p->parent_ip = parent_ip;
  } else {
    /* copy parameters from the caller instrument into our subinstrument */
    lcurip = p->ip;
  }

  lcurip->esr = CS_ESR;
  lcurip->pidsr = CS_PIDSR;
  lcurip->sicvt = CS_SICVT;
  lcurip->onedsr = CS_ONEDSR;
  lcurip->ksmps = CS_KSMPS;
  lcurip->kcounter = CS_KCNT;
  lcurip->ekr = CS_EKR;
  lcurip->onedkr = CS_ONEDKR;
  lcurip->onedksmps = CS_ONEDKSMPS;
  lcurip->kicvt = CS_KICVT;

 

  /* VL 13-12-13 */
  /* this sets ksmps and kr local variables */
  /* create local ksmps variable and init with ksmps */
  if (lcurip->lclbas != NULL) {   
    CS_VARIABLE *var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "ksmps");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ksmps;
    /* same for kr */
    var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "kr");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ekr;
    /* VL 15-08-24 same for sr */
    var =
      csoundFindVariableWithName(csound, lcurip->instr->varPool, "sr");
    *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->esr;
  }

  lcurip->m_chnbp = parent_ip->m_chnbp;       /* MIDI parameters */
  lcurip->m_pitch = parent_ip->m_pitch;
  lcurip->m_veloc = parent_ip->m_veloc;
  lcurip->xtratim = parent_ip->xtratim;
  lcurip->m_sust = 0;
  lcurip->relesing = parent_ip->relesing;
  lcurip->offbet = parent_ip->offbet;
  lcurip->offtim = parent_ip->offtim;
  lcurip->nxtolap = NULL;
  lcurip->ksmps_offset = parent_ip->ksmps_offset;
  lcurip->ksmps_no_end = parent_ip->ksmps_no_end;
  lcurip->tieflag = parent_ip->tieflag;
  lcurip->reinitflag = parent_ip->reinitflag;
  /* copy all p-fields, including p1 (will this work ?) */
  if (tp->pmax > 3) {         /* requested number of p-fields */
    uint32 n = tp->pmax, pcnt = 0;
    while (pcnt < n) {
      if ((i = csound->engineState.instrtxtp[parent_ip->insno]->pmax) > pcnt) {
        if (i > n) i = n;
        /* copy next block of p-fields */
        memcpy(&(lcurip->p1) + pcnt, &(parent_ip->p1) + pcnt,
               (size_t) ((i - pcnt) * sizeof(CS_VAR_MEM)));
        pcnt = i;
      }
      /* top level instr reached */
      if (parent_ip->opcod_iobufs == NULL) break;
      parent_ip = ((OPCOD_IOBUFS*) parent_ip->opcod_iobufs)->parent_ip;
    }
  } else {
    memcpy(&(lcurip->p1), &(parent_ip->p1), 3 * sizeof(CS_VAR_MEM));
  }

  inm->passByRef = buf->opcode_info->newStyle &&
    parent_ip->ksmps == p->ip->ksmps &&
    parent_ip->esr == p->ip->esr;


  if(inm->passByRef) {
    handle_pass_by_ref(csound, p, lcurip);
  }

  /* do init pass for this instr */
  csound->curip = lcurip;
  csound->ids = (OPDS *) (lcurip->nxti);
  ATOMIC_SET(p->ip->init_done, 0);
  csound->mode = 1;
  buf->iflag = 0;
  int err = 0;
  while (csound->ids != NULL && err == 0) {
    csound->op = csound->ids->optext->t.oentry->opname;
    err = (*csound->ids->init)(csound, csound->ids);
    csound->ids = csound->ids->nxti;
  }
    
  if(err) return err; 
  csound->mode = 0;
  ATOMIC_SET(p->ip->init_done, 1);
  /* copy length related parameters back to caller instr */
  parent_ip->relesing = lcurip->relesing;
  parent_ip->offbet = lcurip->offbet;
  parent_ip->offtim = lcurip->offtim;
  parent_ip->p3 = lcurip->p3;

  /* restore globals */
  csound->ids = saved_ids;
  csound->curip = parent_ip;

  /* ksmps and esr may have changed, check against insdshead
     select perf routine and scale xtratim accordingly.
     4 cases:
     (0) passByRef: select useropcd_passByRef
     (1) local ksmps; local sr == parent sr: select useropcd1
     (2) local ksmps; local sr < parent sr: select useropcd2
     (3) local sr >= parent sr: select useropcd2
  */
  
  if(inm->passByRef) {
    parent_ip->xtratim = lcurip->xtratim;
    p->h.perf = (SUBR) useropcd_pass_by_ref;
  } else if (lcurip->ksmps != parent_ip->ksmps &&
	     lcurip->esr == parent_ip->esr) {
    MYFLT ksmps_scale = (MYFLT) lcurip->ksmps / parent_ip->ksmps;
    parent_ip->xtratim = lcurip->xtratim * ksmps_scale;
    // (1) local sr == parent sr
    p->h.perf = (SUBR) useropcd_local_ksmps;
  } else if (lcurip->esr < parent_ip->esr) {
    // (2) local sr < parent sr
      int32_t xtratim = lcurip->xtratim;
      if(parent_ip->xtratim < xtratim)
        parent_ip->xtratim = xtratim; 
      p->h.perf = (SUBR) useropcd_pass_by_copy;
  } else {
    // (3) local sr >= parent sr
    MYFLT scal = (MYFLT) parent_ip->esr / lcurip->esr;
    int32_t xtratim = lcurip->xtratim*scal;
    if(parent_ip->xtratim < xtratim)
	parent_ip->xtratim = xtratim;
    p->h.perf = (SUBR) useropcd_pass_by_copy;
  }
  // debug msg
   if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, "EXTRATIM=> cur(%p): %d, parent(%p): %d\n",
                    lcurip, lcurip->xtratim, parent_ip, parent_ip->xtratim);
  return OK;
}

int32_t useropcd(CSOUND *csound, UOPCODE *p)
{

  if (UNLIKELY(p->h.nxtp))
    return csoundPerfError(csound, &(p->h), Str("%s: not initialised"),
                           p->h.optext->t.opcod);
  else
    return OK;
}

/** This function sets up the input 
    buffers for a UDO in pass-by-copy.
    If oversampling is set *after* xin, then
    this is called again to set up the oversampling
    buffers.
*/

int32_t set_inbufs(CSOUND *csound,
                   OPDS *h,
                   OPCOD_IOBUFS *buf) {
  OPCODINFO   *inm;
  MYFLT **bufs, **tmp;
  int32_t i, k = 0;
  CS_VARIABLE* current;
  UOPCODE  *udo;
  MYFLT parent_sr = buf->parent_ip->esr;
  MYFLT o1ksmps = h->insdshead->ekr/parent_sr;
  MYFLT esr = h->insdshead->esr;
  MYFLT **args = buf->inargs;

  buf->iflag = 1;
  inm = buf->opcode_info;
  udo = (UOPCODE*) buf->uopcode_struct;
  bufs = udo->ar + inm->outchns;
  tmp = buf->iobufp_ptrs; // this is used to record the UDO's internal vars
  // for copying at perf-time
  current = inm->in_arg_pool->head;

  if(inm->passByRef) {
    // printf("New-style UDO using pass-by-ref, skipping...\n");
    return OK;
  }

  for (i = 0; i < inm->inchns; i++) {
    void* in = (void*)bufs[i];
    void* out = (void*) args[i];
    tmp[i + inm->outchns] = out;
    // DO NOT COPY K or A vars
    // Fsigs need to be copied for initialization purposes.
    // check output kvars in case inputs are constants
    if (csoundGetTypeForArg(out) != &CS_VAR_TYPE_K &&
        csoundGetTypeForArg(out) != &CS_VAR_TYPE_A) {
      current->varType->copyValue(csound, current->varType, out, in, h->insdshead);
    }
    else if (csoundGetTypeForArg(out) == &CS_VAR_TYPE_A) {
      // initialise the converter
      if(esr != parent_sr) {
        if((udo->cvt_in[k++] = src_init(csound, h->insdshead->in_cvt,
                                        o1ksmps, h->insdshead->ksmps)) == NULL)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    }
    else if(csoundGetTypeForArg(out) == &CS_VAR_TYPE_K) {
      // initialise the converter
      if(esr != parent_sr) {
        if((udo->cvt_in[k++] = src_init(csound, h->insdshead->in_cvt,
                                        o1ksmps, 1)) == NULL)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    }
    // protect against audio/k arrays when oversampling
    if (csoundGetTypeForArg(in) == &CS_VAR_TYPE_ARRAY) {
      if((current->subType == &CS_VAR_TYPE_A ||
          current->subType == &CS_VAR_TYPE_K)
         && esr != parent_sr)
        return csound->InitError(csound, "audio/control arrays not allowed\n"
                                 "as UDO arguments when using under/oversampling\n");
    }
    current = current->next;
  }

  return OK;
}



int32_t xinset(CSOUND *csound, XIN *p)
{
  OPCOD_IOBUFS *buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
  buf->inargs =  p->args;
  return set_inbufs(csound, &(p->h),
                    buf);
}

int32_t xoutset(CSOUND *csound, XOUT *p)
{
  OPCOD_IOBUFS  *buf;
  OPCODINFO   *inm;
  MYFLT       **bufs, **tmp;
  CS_VARIABLE* current;
  UOPCODE  *udo;
  int32_t i, k = 0;
  MYFLT parent_sr;

  (void) csound;
  buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
  parent_sr = buf->parent_ip->esr;
  inm = buf->opcode_info;
  udo = (UOPCODE*) buf->uopcode_struct;
  bufs = udo->ar;
  tmp = buf->iobufp_ptrs; // this is used to record the UDO's internal vars
  // for copying at perf-time
  current = inm->out_arg_pool->head;

  if(inm->passByRef) {
    // printf("New-style UDO using pass-by-ref, skipping...\n");
    return OK;
  }

  for (i = 0; i < inm->outchns; i++) {
    void* in = (void*) p->args[i];
    void* out = (void*) bufs[i];
    tmp[i] = in;
    // DO NOT COPY K or A vars
    // Fsigs need to be copied for initialization purposes.
    // check output types in case of constants
    if (csoundGetTypeForArg(out) != &CS_VAR_TYPE_K &&
        csoundGetTypeForArg(out) != &CS_VAR_TYPE_A)
      current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
    else if (csoundGetTypeForArg(out) == &CS_VAR_TYPE_A) {
      // initialise the converter
      if(CS_ESR != parent_sr) {
        if((udo->cvt_out[k++] = src_init(csound, p->h.insdshead->out_cvt,
                                         parent_sr/CS_ESR, CS_KSMPS)) == 0)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    }
    else if (csoundGetTypeForArg(out) == &CS_VAR_TYPE_K) {
      // initialise the converter
      if(CS_ESR != parent_sr) {
        if((udo->cvt_out[k++] = src_init(csound, p->h.insdshead->out_cvt,
                                         parent_sr/CS_ESR, 1)) == 0)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    }
    // protect against audio/k arrays when oversampling
    if (csoundGetTypeForArg(in) == &CS_VAR_TYPE_ARRAY) {
      if((current->subType == &CS_VAR_TYPE_A ||
          current->subType == &CS_VAR_TYPE_K)
         && CS_ESR != parent_sr)
        return csound->InitError(csound, "audio/control arrays not allowed\n"
                                 "as UDO arguments when using under/oversampling\n");
    }
    current = current->next;
  }
  return OK;
}


// local ksmps and global sr, pass-by-copy
int32_t useropcd_local_ksmps(CSOUND *csound, UOPCODE *p)
{
  int32_t    g_ksmps, ofs, early, offset, i;
  OPDS *opstart;
  OPCODINFO   *inm;
  CS_VARIABLE* current;
  INSDS    *this_instr = p->ip;
  MYFLT** internal_ptrs = p->buf->iobufp_ptrs;
  MYFLT** external_ptrs = p->ar;
  int32_t done;

  done = ATOMIC_GET(p->ip->init_done);
  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
  early = p->h.insdshead->ksmps_no_end;
  offset = p->h.insdshead->ksmps_offset;
  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;
  inm = p->buf->opcode_info;

  /* global ksmps is the caller instr ksmps minus sample-accurate end */
  g_ksmps = CS_KSMPS - early;

  /* sample-accurate offset */
  ofs = offset;

  if (this_instr->ksmps == 1) {           /* special case for local kr == sr */
    /* clear offsets, since with CS_KSMPS=1
       they don't apply to opcodes, but to the
       calling code (ie. this code)
    */
    this_instr->ksmps_offset = 0;
    this_instr->ksmps_no_end = 0;
    do {
      this_instr->kcounter++; /*kcounter needs to be incremented BEFORE perf */
      /* copy inputs */
      current = inm->in_arg_pool->head;
      for (i = 0; i < inm->inchns; i++) {
        // this hardcoded type check for non-perf time vars needs to change
        //to use generic code...
        // skip a-vars for now, handle uniquely within performance loop
        if (current->varType != &CS_VAR_TYPE_I &&
            current->varType != &CS_VAR_TYPE_b &&
            current->varType != &CS_VAR_TYPE_A &&
            current->subType != &CS_VAR_TYPE_I &&
            current->subType != &CS_VAR_TYPE_A) {
          // This one checks if an array has a subtype of 'i'
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
        } else if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)external_ptrs[i + inm->outchns];
          MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
          *out = *(in + ofs);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
          ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int32_t memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            *out = *(in + ofs);
          }
        }
        current = current->next;
      }

      if ((opstart = (OPDS *) (this_instr->nxtp)) != NULL) {
        int32_t error = 0;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
          opstart->insdshead->pds = opstart;
          error = (*opstart->perf)(csound, opstart);
          opstart = opstart->insdshead->pds;
        } while (error == 0 && p->ip != NULL
                 && (opstart = opstart->nxtp));
      }

      /* copy a-sig outputs, accounting for offset */
      current = inm->out_arg_pool->head;
      for (i = 0; i < inm->outchns; i++) {
        if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)internal_ptrs[i];
          MYFLT* out = (void*)external_ptrs[i];
          *(out + ofs) = *in;
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
          ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int32_t memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            *(out + ofs) = *in;
          }
        }

        current = current->next;
      }

      this_instr->spout += csound->nchnls;
      this_instr->spin  += csound->nchnls;
    } while (++ofs < g_ksmps);
  }
  else {
    /* generic case for local kr != sr */
    /* we have to deal with sample-accurate code
       whole CS_KSMPS blocks are offset here, the
       remainder is left to each opcode to deal with.
    */
    int32_t start = 0;
    int32_t lksmps = this_instr->ksmps;
    while (ofs >= lksmps) {
      ofs -= lksmps;
      start++;
    }
    this_instr->ksmps_offset = ofs;
    ofs = start;
    if (UNLIKELY(early)) this_instr->ksmps_no_end = early % lksmps;
    do {
      this_instr->kcounter++;
      /* copy a-sig inputs, accounting for offset */
      size_t asigSize = (this_instr->ksmps * sizeof(MYFLT));
      current = inm->in_arg_pool->head;
      for (i = 0; i < inm->inchns; i++) {
        // this hardcoded type check for non-perf time vars needs to change
        // to use generic code...
        // skip a-vars for now, handle uniquely within performance loop
        if (current->varType != &CS_VAR_TYPE_I &&
            current->varType != &CS_VAR_TYPE_b &&
            current->varType != &CS_VAR_TYPE_A &&
            current->subType != &CS_VAR_TYPE_I &&
            current->subType != &CS_VAR_TYPE_A) {
          // This one checks if an array has a subtype of 'i'
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
        } else if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)external_ptrs[i + inm->outchns];
          MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
          memcpy(out, in + ofs, asigSize);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
          ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }

          for (j = 0; j < count; j++) {
            int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            memcpy(out, in + ofs, asigSize);
          }
        }
        current = current->next;
      }

      this_instr->ksmps_offset = 0; /* reset sample-accuracy offset for UDO */
      this_instr->ksmps_no_end = 0;  /* reset end of loop samples for UDO */

      /*  run each opcode  */
      if ((opstart = (OPDS *) (this_instr->nxtp)) != NULL) {
        int32_t error = 0;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
          opstart->insdshead->pds = opstart;
          error = (*opstart->perf)(csound, opstart);
          opstart = opstart->insdshead->pds;
        } while (error == 0 && p->ip != NULL
                 && (opstart = opstart->nxtp));
      }

      /* copy a-sig outputs, accounting for offset */
      current = inm->out_arg_pool->head;
      for (i = 0; i < inm->outchns; i++) {
        if (current->varType == &CS_VAR_TYPE_A) {
          MYFLT* in = (void*)internal_ptrs[i];
          MYFLT* out = (void*)external_ptrs[i];
          memcpy(out + ofs, in, asigSize);
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
          ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
          int32_t count = src->sizes[0];
          int32_t j;
          if (src->dimensions > 1) {
            for (j = 0; j < src->dimensions; j++) {
              count *= src->sizes[j];
            }
          }
          for (j = 0; j < count; j++) {
            int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
            MYFLT* in = src->data + memberOffset;
            MYFLT* out = target->data + memberOffset;
            memcpy(out + ofs, in, asigSize);
          }

        }
        current = current->next;
      }

      this_instr->spout += csound->nchnls*lksmps;
      this_instr->spin  += csound->nchnls*lksmps;

    } while ((ofs += this_instr->ksmps) < g_ksmps);
  }


  /* copy outputs */
  current = inm->out_arg_pool->head;
  for (i = 0; i < inm->outchns; i++) {
    // this hardcoded type check for non-perf time vars needs to change
    // to use generic code...
    if (current->varType != &CS_VAR_TYPE_I &&
        current->varType != &CS_VAR_TYPE_b &&
        current->subType != &CS_VAR_TYPE_I) {
      void* in = (void*)internal_ptrs[i];
      void* out = (void*)external_ptrs[i];

      if (current->varType == &CS_VAR_TYPE_A) {
        /* clear the beginning portion of outputs for sample accurate end */
        if (offset) {
          memset(out, '\0', sizeof(MYFLT) * offset);
        }

        /* clear the end portion of outputs for sample accurate end */
        if (early) {
          memset((char*)out + g_ksmps, '\0', sizeof(MYFLT) * early);
        }
      } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                 current->subType == &CS_VAR_TYPE_A) {
        if (offset || early) {
          ARRAYDAT* outDat = (ARRAYDAT*)out;
          int32_t count = outDat->sizes[0];
          int32_t j;
          if (outDat->dimensions > 1) {
            for (j = 0; j < outDat->dimensions; j++) {
              count *= outDat->sizes[j];
            }
          }

          if (offset) {
            for (j = 0; j < count; j++) {
              int memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
              MYFLT* outMem = outDat->data + memberOffset;
              memset(outMem, '\0', sizeof(MYFLT) * offset);
            }
          }

          if (early) {
            for (j = 0; j < count; j++) {
              int32_t memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
              MYFLT* outMem = outDat->data + memberOffset;
              memset(outMem + g_ksmps, '\0', sizeof(MYFLT) * early);
            }
          }
        }
      } else {
        current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
      }
    }
    current = current->next;
  }
 endop:
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)                                         /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) CS_PDS = CS_PDS->nxtp;

  return OK;
}

// global ksmps and global or local sr, pass-by-copy
int32_t useropcd_pass_by_copy(CSOUND *csound, UOPCODE *p)
{
  MYFLT   **tmp;
  OPCODINFO   *inm;
  CS_VARIABLE* current;
  int32_t i, done;
  int32_t os = (int) (p->ip->esr/p->parent_ip->esr);

  inm = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
  done = ATOMIC_GET(p->ip->init_done);

  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  /* VL 18.12.24: ksmps_no_end is copied here as it
     applies only to last kcycle */ 
  p->ip->ksmps_no_end = p->h.insdshead->ksmps_no_end;
  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;

  if (UNLIKELY(p->ip->nxtp == NULL))
    goto endop; /* no perf code */

  p->ip->relesing = p->parent_ip->relesing;
  tmp = p->buf->iobufp_ptrs;
  inm = p->buf->opcode_info;

  MYFLT** internal_ptrs = tmp;
  MYFLT** external_ptrs = p->ar;
  int32_t ocnt = 0;

  /*  run each opcode, oversampling if necessary  */
  for(ocnt = 0; ocnt < os; ocnt++){
    int error = 0;
    int cvt;
    OPDS *opstart;
    /* copy inputs */
    current = inm->in_arg_pool->head;
    for (i = cvt = 0; i < inm->inchns; i++) {
      // this hardcoded type check for non-perf time vars needs to
      // change to use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if(os == 1) {
          if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
            *internal_ptrs[i + inm->outchns] = *external_ptrs[i + inm->outchns];
          } else {
            void* in = (void*)external_ptrs[i + inm->outchns];
            void* out = (void*)internal_ptrs[i + inm->outchns];
            current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
          }
        } else { // oversampling
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          if (current->varType == &CS_VAR_TYPE_A ||
              current->varType == &CS_VAR_TYPE_K) {
            // sample rate conversion
            src_convert(csound, p->cvt_in[cvt++], in, out);

          }
          else if(ocnt == 0) // only copy other variables once
            current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
        }
      }
      current = current->next;
    }

    if ((opstart = (OPDS *) (p->ip->nxtp)) != NULL) {
      p->ip->kcounter++;  /* kcount should be incremented BEFORE perf */
      do {
        if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
        opstart->insdshead->pds = opstart;
        error = (*opstart->perf)(csound, opstart);
        opstart = opstart->insdshead->pds;
      } while (error == 0 && p->ip != NULL
               && (opstart = opstart->nxtp));
    }

    /* copy outputs */
    current = inm->out_arg_pool->head;
    for (i = cvt = 0; i < inm->outchns; i++) {
      // this hardcoded type check for non-perf time vars needs to change to
      // use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if(os == 1) {
          if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
            *external_ptrs[i] = *internal_ptrs[i];
          } else {
            void* in = (void*)internal_ptrs[i];
            void* out = (void*)external_ptrs[i];
            current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
          }
        }
        else { // oversampling
          void* in = (void*)internal_ptrs[i];
          void* out = (void*)external_ptrs[i];
          if (current->varType == &CS_VAR_TYPE_A ||
              current->varType == &CS_VAR_TYPE_K) {
            // sample rate conversion
            src_convert(csound, p->cvt_out[cvt++], in, out);
          } else if(ocnt == 0) {// only copy other variables once
            current->varType->copyValue(csound, current->varType, out, in, p->h.insdshead);
          }
        }
      }
      current = current->next;
    }
  }

 endop:
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)  {                   /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  if(p->ip) { // check in case instrument has called turnoff
  p->ip->ksmps_offset = 0; /* reset sample-accuracy offset */
  p->ip->ksmps_no_end = 0;  /* reset end of loop samples */
  }
  return OK;
}

/** Runs perf-time chain*/
int32_t useropcd_pass_by_ref(CSOUND *csound, UOPCODE *p)
{
  OPDS    *saved_pds = CS_PDS;
  int32_t done;

  done = ATOMIC_GET(p->ip->init_done);

  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  p->ip->spin = p->parent_ip->spin;
  p->ip->spout = p->parent_ip->spout;
  p->ip->kcounter++;  /* kcount should be incremented BEFORE perf */

  if (UNLIKELY(!(CS_PDS = (OPDS*) (p->ip->nxtp))))
    goto endop; /* no perf code */

  /* IV - Nov 16 2002: update release flag */
  p->ip->relesing = p->parent_ip->relesing;

  /*  run each opcode  */
  {
  int error = 0;
  CS_PDS->insdshead->pds = NULL;
  do {
    if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))) goto endop;
    error = (*CS_PDS->perf)(csound, CS_PDS);
    if (CS_PDS->insdshead->pds != NULL &&
        CS_PDS->insdshead->pds->insdshead) {
      CS_PDS = CS_PDS->insdshead->pds;
      CS_PDS->insdshead->pds = NULL;
    }
  } while (error == 0 && p->ip != NULL
           && (CS_PDS = CS_PDS->nxtp));
  }

 endop:

  /* restore globals */
  CS_PDS = saved_pds;
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip)  {                   /* loop to last opds */
    while (CS_PDS && CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  return OK;
}

/*
  This opcode sets the local ksmps for an instrument
  it can be used on any instrument with the implementation
  of a mechanism to perform at local ksmps (in kperf etc)
*/
int32_t setksmpsset(CSOUND *csound, SETKSMPS *p)
{

  uint32_t  l_ksmps, n;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr = udo ? udo->parent_ip->esr : csound->esr;
  MYFLT parent_ksmps = udo ? udo->parent_ip->ksmps : csound->ksmps;

  if(CS_ESR != parent_sr)
    return csoundInitError(csound,
                           "can't set ksmps value if local sr != parent sr\n");
  if(CS_KSMPS != parent_ksmps) return OK; // no op if this has already changed

  l_ksmps = (uint32_t) *(p->i_ksmps);
  if (!l_ksmps) return OK;       /* zero: do not change */
  if (UNLIKELY(l_ksmps < 1 || l_ksmps > CS_KSMPS ||
               ((CS_KSMPS / l_ksmps) * l_ksmps != CS_KSMPS))) {
    return csoundInitError(csound,
                           Str("setksmps: invalid ksmps value: %d, original: %d"),
                           l_ksmps, CS_KSMPS);
  }

  n = CS_KSMPS / l_ksmps;
  p->h.insdshead->xtratim *= n;
  CS_KSMPS = l_ksmps;
  CS_ONEDKSMPS = FL(1.0) / (MYFLT) CS_KSMPS;
  CS_EKR = CS_ESR / (MYFLT) CS_KSMPS;
  CS_ONEDKR = FL(1.0) / CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;
  CS_KCNT *= n;

  /* VL 13-12-13 */
  /* this sets ksmps and kr local variables */
  /* lookup local ksmps variable and init with ksmps */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "ksmps");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_KSMPS;

  /* same for kr */
  var =
    csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;

  return OK;
}

/* oversample opcode
   oversample ifactor
   ifactor - oversampling factor (positive integer)

   if oversampling is used, xin/xout need
   to initialise the converters.
   oversampling is not allowed with local ksmps or
   with audio/control array arguments.
*/
int32_t oversampleset(CSOUND *csound, OVSMPLE *p) {
  int32_t os;
  MYFLT l_sr, onedos;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr, parent_ksmps;

  if(udo == NULL)
    return csound->InitError(csound, "oversampling only allowed in UDOs\n");

  parent_sr = udo->parent_ip->esr;
  parent_ksmps = udo->parent_ip->ksmps;

  if(CS_KSMPS != parent_ksmps)
    return csoundInitError(csound,
                           "can't oversample if local ksmps != parent ksmps\n");

  os = MYFLT2LRND(*p->os);
  onedos = FL(1.0)/os;
  if(os < 1)
    return csound->InitError(csound, "illegal oversampling ratio: %d\n", os);
  if(os == 1 || CS_ESR != parent_sr) return OK; /* no op if changed already */

  l_sr = CS_ESR*os;
  CS_ESR = l_sr;
  CS_PIDSR = PI/l_sr;
  CS_ONEDSR = 1./l_sr;
  CS_SICVT = (MYFLT) FMAXLEN / CS_ESR;
  CS_EKR = CS_ESR/CS_KSMPS;
  CS_ONEDKR = 1./CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;
  /* ksmsp does not change,
     however, because we are oversampling, we will need
     to run the code os times in a loop to consume
     os*ksmps input samples and produce os*ksmps output
     samples. This means that the kcounter will run fast by a
     factor of 1/os, and xtratim also needs to be scaled by
     that factor
  */
  p->h.insdshead->xtratim *= os;
  CS_KCNT *= onedos;
  /* oversampling mode (s) */
  p->h.insdshead->in_cvt = MYFLT2LRND(*p->in_cvt);
  if(*p->out_cvt >= 0)
    p->h.insdshead->out_cvt = MYFLT2LRND(*p->out_cvt);
  else p->h.insdshead->out_cvt = p->h.insdshead->in_cvt;
  /* set local sr variable */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "sr");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_ESR;
  var = csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;

  if(udo->iflag) // if xin has already been called, reset bufs
    return set_inbufs(csound, &(p->h), udo);
  else return OK;
}

/* undersample opcode
   undersample ifactor
   ifactor - undersampling factor (positive integer)

   if ubdersampling is used, xin/xout need
   to initialise the converters.
   undersampling is not allowed with
   with audio/control array arguments.
   It modifies ksmps according to the resampling factor.
*/
int32_t undersampleset(CSOUND *csound, OVSMPLE *p) {
  int32_t os, lksmps;
  MYFLT l_sr, onedos;
  OPCOD_IOBUFS *udo = (OPCOD_IOBUFS *) p->h.insdshead->opcod_iobufs;
  MYFLT parent_sr, parent_ksmps;

  if(udo == NULL)
    return csound->InitError(csound, "oversampling only allowed in UDOs\n");

  parent_sr = udo->parent_ip->esr;
  parent_ksmps = udo->parent_ip->ksmps;

  if(CS_KSMPS != parent_ksmps)
    return csoundInitError(csound,
                           "can't undersample if local ksmps != parent ksmps\n");

  os = MYFLT2LRND(*p->os);
  onedos = FL(1.0)/os;
  if(os < 1)
    return csound->InitError(csound,
                             "illegal undersampling ratio: %d\n", os);

  if(os == 1 || CS_ESR != parent_sr) return OK; /* no op if already changed */

  /* round to an integer number of ksmps */
  lksmps = MYFLT2LRND(CS_KSMPS*onedos);
  /* and check */
  if(lksmps < 1)
    return csound->InitError(csound,
                             "illegal oversampling ratio: %d\n", os);

  /* set corrected ratio  */
  onedos = (MYFLT) lksmps/CS_KSMPS;

  /* and now local ksmps */
  CS_KSMPS = lksmps;
  CS_ONEDKSMPS = FL(1.0)/lksmps;
  l_sr = CS_ESR*onedos;
  CS_ESR = l_sr;
  CS_PIDSR = PI/l_sr;
  CS_ONEDSR = 1./l_sr;
  CS_SICVT = (MYFLT) FMAXLEN / CS_ESR;
  CS_EKR = CS_ESR/CS_KSMPS;
  CS_ONEDKR = 1./CS_EKR;
  CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;

  p->h.insdshead->xtratim *= onedos;
  CS_KCNT *= FL(1.0)/onedos;
  /* undersampling mode (s) */
  p->h.insdshead->in_cvt = MYFLT2LRND(*p->in_cvt);
  if(*p->out_cvt >= 0)
    p->h.insdshead->out_cvt = MYFLT2LRND(*p->out_cvt);
  else p->h.insdshead->out_cvt = p->h.insdshead->in_cvt;
  /* set local sr variable */
  INSTRTXT *ip = p->h.insdshead->instr;
  CS_VARIABLE *var =
    csoundFindVariableWithName(csound, ip->varPool, "sr");
  MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_ESR;
  var = csoundFindVariableWithName(csound, ip->varPool, "kr");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_EKR;
  var = csoundFindVariableWithName(csound, ip->varPool, "ksmps");
  varmem = p->h.insdshead->lclbas + var->memBlockIndex;
  *varmem = CS_KSMPS;

  if(udo->iflag) // if xin has already been called, reset bufs
    return set_inbufs(csound, &(p->h), udo);
  else return OK;
}


/*
 * subinstr opcode
 */
int32_t subinstrset_(CSOUND *csound, SUBINST *p, int32_t instno)
{
  OPDS    *saved_ids = csound->ids;
  INSDS   *saved_curip = csound->curip;
  CS_VAR_MEM   *pfield;
  int32_t     n, init_op, inarg_ofs;
  INSDS  *pip = p->h.insdshead;

  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  if (UNLIKELY(instno < 0)) return NOTOK;
  /* IV - Oct 9 2002: need this check */
  if (UNLIKELY(!init_op && p->OUTOCOUNT > csound->nchnls)) {
    return csoundInitError(csound, "%s",Str("subinstr: number of output "
                                       "args greater than nchnls"));
  }
  /* IV - Oct 9 2002: copied this code from useropcdset() to fix some bugs */
  if (!(pip->reinitflag | pip->tieflag) || p->ip == NULL) {
    /* get instance */
    if (csound->engineState.instrtxtp[instno]->act_instance == NULL)
      instance(csound, instno);
    p->ip = csound->engineState.instrtxtp[instno]->act_instance;
    csound->engineState.instrtxtp[instno]->act_instance = p->ip->nxtact;
    p->ip->insno = (int16) instno;
    p->ip->actflg++;                  /*    and mark the instr active */
    csound->engineState.instrtxtp[instno]->active++;
    csound->engineState.instrtxtp[instno]->instcnt++;
    p->ip->p1.value = (MYFLT) instno;
    /* VL 21-10-16: iobufs are not used here and
       are causing trouble elsewhere. Commenting
       it out */
    /* p->ip->opcod_iobufs = (void*) &p->buf; */
    /* link into deact chain */
    p->ip->subins_deact = saved_curip->subins_deact;
    p->ip->opcod_deact = NULL;
    saved_curip->subins_deact = (void*) p;
    p->parent_ip = p->buf.parent_ip = saved_curip;
  }

  p->ip->esr = CS_ESR;
  p->ip->pidsr = CS_PIDSR;
  p->ip->sicvt = CS_SICVT;
  p->ip->onedsr = CS_ONEDSR;
  p->ip->ksmps = CS_KSMPS;
  p->ip->kcounter = CS_KCNT;
  p->ip->ekr = CS_EKR;
  p->ip->onedkr = CS_ONEDKR;
  p->ip->onedksmps = CS_ONEDKSMPS;
  p->ip->kicvt = CS_KICVT;

  /* copy parameters from this instrument into our subinstrument */
  p->ip->xtratim  = saved_curip->xtratim;
  p->ip->m_sust   = 0;
  p->ip->relesing = saved_curip->relesing;
  p->ip->offbet   = saved_curip->offbet;
  p->ip->offtim   = saved_curip->offtim;
  p->ip->nxtolap  = NULL;
  p->ip->p2       = saved_curip->p2;
  p->ip->p3       = saved_curip->p3;

  /* IV - Oct 31 2002 */
  p->ip->m_chnbp  = saved_curip->m_chnbp;
  p->ip->m_pitch  = saved_curip->m_pitch;
  p->ip->m_veloc  = saved_curip->m_veloc;

  p->ip->ksmps_offset =  saved_curip->ksmps_offset;
  p->ip->ksmps_no_end =  saved_curip->ksmps_no_end;
  p->ip->tieflag = saved_curip->tieflag;
  p->ip->reinitflag = saved_curip->reinitflag;

  /* copy remainder of pfields */
  pfield = (CS_VAR_MEM*)&p->ip->p3;
  /* by default all inputs are i-rate mapped to p-fields */
  if (UNLIKELY(p->INOCOUNT >
               (unsigned int)(csound->engineState.instrtxtp[instno]->pmax + 1)))
    return csoundInitError(csound, "%s", Str("subinstr: too many p-fields"));
#ifdef USE_DOUBLE
  union {
    MYFLT d;
    int32 i[2];
  } ch;
  int32_t sel = byte_order()==0? 1 :0;
  int32_t str_cnt = 0, len = 0;
  char *argstr;
  for (n = 1; (uint32_t) n < p->INOCOUNT; n++){
    if (IS_STR_ARG(p->ar[inarg_ofs + n])) {
      ch.d = SSTRCOD;
      ch.i[sel] += str_cnt & 0xffff;
      (pfield + n)->value = ch.d;
      argstr = ((STRINGDAT *)p->ar[inarg_ofs + n])->data;
      if (str_cnt == 0)
        p->ip->strarg = csound->Calloc(csound, strlen(argstr)+1);
      else
        p->ip->strarg = csound->ReAlloc(csound, p->ip->strarg,
                                        len+strlen(argstr)+1);
      strcpy(p->ip->strarg + len, argstr);
      len += strlen(argstr)+1;
      str_cnt++;
    }

    else (pfield + n)->value = *p->ar[inarg_ofs + n];
  }
#else
  union {
    MYFLT d;
    int32 j;
  } ch;
  int32_t str_cnt = 0, len = 0;
  char *argstr;
  for (n = 1; (uint32_t) n < p->INOCOUNT; n++){
    if (IS_STR_ARG(p->ar[inarg_ofs + n])) {
      ch.d = SSTRCOD;
      ch.j += str_cnt & 0xffff;
      (pfield + n)->value = ch.d;
      argstr = ((STRINGDAT *)p->ar[inarg_ofs + n])->data;
      if (str_cnt == 0)
        p->ip->strarg = csound->Calloc(csound, strlen(argstr)+1);
      else
        p->ip->strarg = csound->ReAlloc(csound, p->ip->strarg,
                                        len+strlen(argstr)+1);
      strcpy(p->ip->strarg + len, argstr);
      len += strlen(argstr)+1;
      str_cnt++;
    }
    else (pfield + n)->value = *p->ar[inarg_ofs + n];
  }
#endif

  // allocate memory for a temporary store of spout buffers
  if (!init_op && !(pip->reinitflag | pip->tieflag))
    csound->AuxAlloc(csound, (int32) csound->nspout * sizeof(MYFLT), &p->saved_spout);

  /* do init pass for this instr */
  csound->curip = p->ip;        /* **** NEW *** */
  p->ip->init_done = 0;
  csound->ids = (OPDS *)p->ip;
  csound->mode = 1;
  while ((csound->ids = csound->ids->nxti) != NULL) {
    csound->op = csound->ids->optext->t.oentry->opname;
    (*csound->ids->init)(csound, csound->ids);
  }
  csound->mode = 0;
  p->ip->init_done = 1;
  /* copy length related parameters back to caller instr */
  saved_curip->xtratim = csound->curip->xtratim;
  saved_curip->relesing = csound->curip->relesing;
  saved_curip->offbet = csound->curip->offbet;
  saved_curip->offtim = csound->curip->offtim;
  saved_curip->p3 = csound->curip->p3;

  /* restore globals */
  csound->ids = saved_ids;
  csound->curip = saved_curip;
  return OK;
}

int32_t subinstrset_S(CSOUND *csound, SUBINST *p){
  int32_t instno, init_op, inarg_ofs;
  /* check if we are using subinstrinit or subinstr */
  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  instno = string_arg_to_insno(csound, ((STRINGDAT *)p->ar[inarg_ofs])->data, 1);
  if (UNLIKELY(instno==NOT_AN_INSTRUMENT)) instno = -1;
  return subinstrset_(csound,p,instno);
}


int32_t subinstrset(CSOUND *csound, SUBINST *p){
  int32_t instno, init_op, inarg_ofs;
  /* check if we are using subinstrinit or subinstr */
  init_op = (p->h.perf == NULL ? 1 : 0);
  inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
  instno = (int32_t) *(p->ar[inarg_ofs]);
  return subinstrset_(csound,p,instno);
}

int32_t subinstr(CSOUND *csound, SUBINST *p)
{
  OPDS    *saved_pds = CS_PDS;
  MYFLT   *pbuf;
  uint32_t frame, chan;
  uint32_t nsmps = CS_KSMPS;
  INSDS *ip = p->ip;
  int32_t done = ATOMIC_GET(p->ip->init_done);

  if (UNLIKELY(!done)) /* init not done, exit */
    return OK;

  if (UNLIKELY(p->ip == NULL)) {                /* IV - Oct 26 2002 */
    return csoundPerfError(csound, &(p->h), "%s",
                           Str("subinstr: not initialised"));
  }

  /* copy current spout buffer and clear it */
  ip->spout = (MYFLT*) p->saved_spout.auxp;
  memset(ip->spout, 0, csound->nspout*sizeof(MYFLT));

  /* update release flag */
  ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
  /*  run each opcode  */
  if (csound->ksmps == ip->ksmps) {
    int32_t error = 0;
    ip->kcounter++;
    if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
      CS_PDS->insdshead->pds = NULL;
      do {
        error = (*CS_PDS->perf)(csound, CS_PDS);
        if (CS_PDS->insdshead->pds != NULL) {
          CS_PDS = CS_PDS->insdshead->pds;
          CS_PDS->insdshead->pds = NULL;
        }
      } while (error == 0 && (CS_PDS = CS_PDS->nxtp));
    }

  }
  else {
    int32_t i, n = csound->nspout, start = 0;
    int32_t lksmps = ip->ksmps;
    int32_t incr = csound->nchnls*lksmps;
    int32_t offset =  ip->ksmps_offset;
    int32_t early = ip->ksmps_no_end;
    ip->spin = csound->spin;
    ip->kcounter =  csound->kcounter*csound->ksmps/lksmps;

    /* we have to deal with sample-accurate code
       whole CS_KSMPS blocks are offset here, the
       remainder is left to each opcode to deal with.
    */
    while (offset >= lksmps) {
      offset -= lksmps;
      start += csound->nchnls;
    }
    ip->ksmps_offset = offset;
    if (early) {
      n -= (early*csound->nchnls);
      ip->ksmps_no_end = early % lksmps;
    }

    for (i=start; i < n; i+=incr, ip->spin+=incr, ip->spout+=incr) {
      ip->kcounter++;
      if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
        int32_t error = 0;
        CS_PDS->insdshead->pds = NULL;
        do {
          if(UNLIKELY(!ATOMIC_GET8(p->ip->actflg))){
            memset(p->ar, 0, sizeof(MYFLT)*CS_KSMPS*p->OUTOCOUNT);
            goto endin;
          }
          error = (*CS_PDS->perf)(csound, CS_PDS);
          if (CS_PDS->insdshead->pds != NULL) {
            CS_PDS = CS_PDS->insdshead->pds;
            CS_PDS->insdshead->pds = NULL;
          }
        } while (error == 0 && (CS_PDS = CS_PDS->nxtp));
      }
    }
    ip->spout = (MYFLT*) p->saved_spout.auxp;
  }
  /* copy outputs */
  for (chan = 0; chan < p->OUTOCOUNT; chan++) {
    for (pbuf = ip->spout + chan*nsmps, frame = 0;
         frame < nsmps; frame++) {
      p->ar[chan][frame] = pbuf[frame];
      //printf("%f\n", p->ar[chan][frame]);
      //pbuf += csound->nchnls;
    }
  }
 endin:
  CS_PDS = saved_pds;
  /* check if instrument was deactivated (e.g. by perferror) */
  if (!p->ip) {                                  /* loop to last opds */
    while (CS_PDS->nxtp) {
      CS_PDS = CS_PDS->nxtp;
    }
  }
  return OK;
}
