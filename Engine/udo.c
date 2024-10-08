#include "udo.h"
#include "Opcodes/biquad.h"
#include "csound_data_structures.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "insert.h"

/* IV - Sep 8 2002: new functions for user defined opcodes (based */
/* on Matt J. Ingalls' subinstruments, but mostly rewritten) */

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

  size_t i;
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
  while (csound->ids != NULL) {
    csound->op = csound->ids->optext->t.oentry->opname;
    (*csound->ids->init)(csound, csound->ids);
    csound->ids = csound->ids->nxti;
  }
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
    p->h.perf = (SUBR) useropcd_passByRef;
  } else if (lcurip->ksmps != parent_ip->ksmps) {
    int32_t ksmps_scale = lcurip->ksmps / parent_ip->ksmps;
    parent_ip->xtratim = lcurip->xtratim * ksmps_scale;
    if(lcurip->esr == parent_ip->esr) // (1) local sr == parent sr
      p->h.perf = (SUBR) useropcd1;
    else // (2) local sr < parent sr
      p->h.perf = (SUBR) useropcd2;
  } else { // (3) local sr >= parent sr
    parent_ip->xtratim = lcurip->xtratim;
    p->h.perf = (SUBR) useropcd2;
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

int32_t xinset(CSOUND *csound, XIN *p)
{
  OPCOD_IOBUFS  *buf;
  OPCODINFO   *inm;
  MYFLT **bufs, **tmp;
  int32_t i, k = 0;
  CS_VARIABLE* current;
  UOPCODE  *udo;
  MYFLT parent_sr;

  (void) csound;
  buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
  buf->iflag = 1;
  parent_sr = buf->parent_ip->esr;
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
    void* out = (void*)p->args[i];
    tmp[i + inm->outchns] = out;
    // DO NOT COPY K or A vars
    // Fsigs need to be copied for initialization purposes.
    // check output kvars in case inputs are constants
    if (csoundGetTypeForArg(out) != &CS_VAR_TYPE_K &&
        csoundGetTypeForArg(out) != &CS_VAR_TYPE_A) {
      current->varType->copyValue(csound, current->varType, out, in, &(p->h));
    }
    else if (csoundGetTypeForArg(out) == &CS_VAR_TYPE_A) {
      // initialise the converter
      if(CS_ESR != parent_sr) {
        if((udo->cvt_in[k++] = src_init(csound, p->h.insdshead->in_cvt,
                                        CS_ESR/parent_sr, CS_KSMPS)) == NULL)
          return csound->InitError(csound, "could not initialise sample rate "
                                   "converter");
      }
    }
    else if(csoundGetTypeForArg(out) == &CS_VAR_TYPE_K) {
      // initialise the converter
      if(CS_ESR != parent_sr) {
        if((udo->cvt_in[k++] = src_init(csound, p->h.insdshead->in_cvt,
                                        CS_ESR/parent_sr, 1)) == NULL)
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
      current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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


// local ksmps and global sr
int32_t useropcd1(CSOUND *csound, UOPCODE *p)
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

  /* clear offsets, since with CS_KSMPS=1
     they don't apply to opcodes, but to the
     calling code (ie. this code)
  */
  this_instr->ksmps_offset = 0;
  this_instr->ksmps_no_end = 0;

  if (this_instr->ksmps == 1) {           /* special case for local kr == sr */
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
          current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
          current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
        current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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

// global ksmps amd global or local sr
int32_t useropcd2(CSOUND *csound, UOPCODE *p)
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
            current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
            current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
            current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
            current->varType->copyValue(csound, current->varType, out, in, &(p->h));
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
  return OK;
}

/** Runs perf-time chain*/
int32_t useropcd_passByRef(CSOUND *csound, UOPCODE *p)
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
