/*  compile_ops.c: compilation and internal OSC server opcodes

    Copyright (c) 2013 Victor Lazzarini

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
*/

#include "compile_ops.h"
#include <stdio.h>
int32_t csound_compile_orc(CSOUND *csound, const char *str, int32_t async);
int32_t csoundReadScoreInternal(CSOUND *csound, const char *str);

int32_t compile_orc_i(CSOUND *csound, COMPILE *p){
  FILE *fp;
  size_t size=0;
  char *orc, c, *name;

  name = ((STRINGDAT *)p->str)->data;
  fp = fopen(name, "rb");

  if (fp == NULL) {
    csound->Warning(csound, Str("compileorc: could not open %s\n"), name);
    *p->res = (MYFLT)(CSOUND_ERROR);
    return NOTOK;
  }

  while(!feof(fp))
    size += fread(&c,1,1,fp);

  if(size==0) {
    fclose(fp);
    *p->res = (MYFLT)(CSOUND_ERROR);
    return
      csound->InitError(csound, Str("compileorc: could not read %s\n"), name);
  }

  orc = (char *) csound->Calloc(csound, size+1);
  fseek(fp, 0, SEEK_SET);
  if (UNLIKELY(fread(orc,1,size,fp)!=size)) {
    fclose(fp);
    csound->Free(csound,orc);
    return NOTOK;
  }
  *p->res = (MYFLT)(csound_compile_orc(csound, orc, 0));
  fclose(fp);
  csound->Free(csound,orc);
  return OK;
}

int32_t compile_csd_i(CSOUND *csound, COMPILE *p){
  *p->res = (MYFLT) csoundCompileCSD(csound, ((STRINGDAT *)p->str)->data, 0, 0);
  return OK;
}

int32_t compile_str_i(CSOUND *csound, COMPILE *p){
  //void csp_orc_sa_print_list(CSOUND*);
  //printf("START\n");
  *p->res = (MYFLT)(csound_compile_orc(csound,
                                       ((STRINGDAT *)p->str)->data, 0));
  //printf("END\n");
  //csp_orc_sa_print_list(csound);
  return OK;
}

/* compiles a single instrument:
   Instr new_instr Scode  -> adds new instr in free slot
*/
int32_t compile_instr(CSOUND *csound, CINSTR *p) {
  INSTRTXT **instrs = csound->GetInstrumentList(csound);
  int32_t num = 1;
  size_t siz;
  char *code;
  const char *endin = "\n endin \n";
  // look for a free slot
  while(instrs[num] != NULL) num++;
  siz = strlen(p->code->data) + strlen(endin) + 16;
  code = csound->Calloc(csound, siz);
  snprintf(code, siz,"instr %d\n%s%s", num, p->code->data, endin);

  if(csound->GetDebug(csound) & DEBUG_OPCODES) csound->Message(csound, "%s \n", code);
  // compile code
  if(csound_compile_orc(csound, code, 0) == CSOUND_SUCCESS) {
    // pass the instrument out
    p->instr->instr = instrs[num];
    csound->Free(csound, code);
    return OK;
  }
  csound->Free(csound, code);
  return csound->InitError(csound, "failed to compile instr\n");
}


int32_t read_score_i(CSOUND *csound, COMPILE *p){
  *p->res = (MYFLT)(csoundReadScoreInternal(csound,
                                            ((STRINGDAT *)p->str)->data));
  return OK;
}

int32_t eval_str_i(CSOUND *csound, COMPILE *p){
  *p->res = csoundEvalCode(csound, ((STRINGDAT *)p->str)->data);
  return OK;
}

int32_t eval_str_k(CSOUND *csound, COMPILE *p){
  if (*p->ktrig)
    *p->res = csoundEvalCode(csound, ((STRINGDAT *)p->str)->data);
  return OK;
}


int32_t retval_i(CSOUND *csound, RETVAL *p){
  IGN(csound);
  INSDS *ip = p->h.insdshead;
  ip->retval = *p->ret;
  return OK;
}

/** Read OSC message from linked list
 */
OSC_MESS *csoundReadOSCMessage(CSOUND *csound, const char *address,
                               const char *type){  
  OSC_MESS *p = &csound->osc_message_anchor;
  spin_lock_t *lock = &csound->osc_spinlock;
  // no messages, just exit
  if(p->address == NULL) return NULL;
  csoundSpinLock(lock);
  do {
    if(p->flag &&
      !strcmp(p->address, address)
       && !strcmp(p->type, type)) break;
  } while((p = p->nxt) != NULL);
  csoundSpinUnLock(lock);
  return p;
}

/** Clear flag for OSC message so its slot can be reused.
 */
void csoundClearOSCMessage(OSC_MESS *mess){
  ATOMIC_SET(mess->flag, 0);
}

/** Get float from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetFloat(const char *buf, MYFLT *mf) {
  float f;
  f = *((float *) buf);
  byteswap((char*)&f,4);
  *mf = (MYFLT) f;
  return buf + 4;
}

const char *csoundOSCMessageGetDouble(const char *buf, MYFLT *mf) {
  double f;
  f = *((double *) buf);
  byteswap((char*)&f,8);
  *mf = (MYFLT) f;
  return buf + 8;
}


/** Get int32_t from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetInt32(const char *buf, MYFLT *mf) {
  int32_t i;
  i = *((int32_t *) buf);
  byteswap((char*)&i,4);
  *mf = (MYFLT) i;
  return buf + 4;
}

/** Get int64 from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetInt64(const char *buf, MYFLT *mf) {
  int64_t i;
  i = *((int64_t *) buf);
  byteswap((char*)&i,8);
  *mf = (MYFLT) i;
  return buf + 8;
}

/** Get char from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetChar(const char *buf, MYFLT *mf) {
  int8_t i;
  i = *((int8_t *) buf);
  *mf = (MYFLT) i;
  return buf + 1;
}

/** Get stringdata from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetString(const char *data, STRINGDAT *sdat) {
  size_t len = strlen(data)+1;
  strncpy(sdat->data, data, sdat->size-1);
  sdat->data[sdat->size-1] = '\0'; // safety
  return data+((size_t) ceil(len/4.)*4);
}

/** Get a number according to type 
    returns pointer to the next datum or NULL on failure
*/
const char *csoundOSCMessageGetNumber(const char *buf,
                                      char type, MYFLT *out) {
  switch(type){
  case 'f':
    buf = csoundOSCMessageGetFloat(buf,out);
    break;
  case 'd':
    buf = csoundOSCMessageGetDouble(buf,out);
    break;
  case 'i':
    buf = csoundOSCMessageGetInt32(buf,out);
    break;
  case 'h':
    buf = csoundOSCMessageGetInt64(buf,out);
    break;
  case 'c':
    buf = csoundOSCMessageGetChar(buf,out);
    break;
  default:  
    return NULL;
  }
  return buf;
}

int32_t readOSC_perf(CSOUND *csound, ROSC *p) {
  int32_t cnt = p->OUTOCOUNT - 1, i;
  if(cnt > 32)
    return csound->PerfError(csound, &(p->h),
                             "OSCRead exceeded max output args (>32)\n");
  OSC_MESS *mess = csoundReadOSCMessage(csound, p->address->data,
                                        p->type->data);
  if(mess != NULL) {
    MYFLT **out = p->out;
    const char *buf = mess->data;
    const char *type = p->type->data;
    for(i = 0; i < cnt; i++) {
      if(type[i] == 's' &&
         IS_STR_ARG(out[i])) {
        buf = csoundOSCMessageGetString(buf, (STRINGDAT *) out[i]);
      }
      else if(IS_KSIG_ARG(p->out[i])){
        buf = csoundOSCMessageGetNumber(buf, type[i], out[i]);
        if(buf == NULL)
          return csound->PerfError(csound, &(p->h),  
                                   "unsupported OSC type %c", type[i]);
      }
      else
        return csound->PerfError(csound, &(p->h), "wrong output argument" 
                                 "for OSC type %c", type[i]);
    }
    *p->kstatus = 1;
    csoundClearOSCMessage(mess);
  }
  return OK;
}

#include "arrays.h"

int32_t readOSCarray_init(CSOUND *csound, ROSCA *p) {
  tabinit(csound, p->out, (int32_t) strlen(p->type->data), p->h.insdshead);
  return OK;
}

int32_t readOSCarray_perf(CSOUND *csound, ROSCA *p) {
  int32_t cnt = p->out->sizes[0], i;
  OSC_MESS *mess = csoundReadOSCMessage(csound, p->address->data,
                                        p->type->data);
  if(mess != NULL) {
    MYFLT *out = p->out->data;
    const char *buf = mess->data;
    const char *type = p->type->data;
    for(i = 0; i < cnt; i++) {
      buf = csoundOSCMessageGetNumber(buf, type[i], &out[i]);
      if(buf == NULL)
        return csound->PerfError(csound, &(p->h),  
                                 "unsupported OSC type %c",
                                 type[i]);
    }
    *p->kstatus = 1;
    csoundClearOSCMessage(mess);
  }
  return OK;
}

#include "aops.h"
int32_t myflt_size(CSOUND *csound, ASSIGN *p) {
  *p->r = FL(sizeof(MYFLT));
  return OK;
}


// Csound Type & Opcodes
typedef struct {
  CSOUND *csound;
  int32_t nsmps;
  MYFLT  *bufferout;
  MYFLT  *bufferin;
} CS_OBJ;

static void csobj_var_init_memory(CSOUND *csound, CS_VARIABLE* var, MYFLT* memblock) {
  memset(memblock, 0, var->memBlockSize);
}

static void csobj_copy_value(CSOUND* csound, const CS_TYPE* cstype, void* dest,
                             const void* src, INSDS *ctx) {
  memcpy(dest, src, sizeof(CS_OBJ));
}

static CS_VARIABLE* create_csobj_var(void* cs, void* p, INSDS *ctx) {
  CSOUND* csound = (CSOUND*) cs;
  CS_VARIABLE* var = (CS_VARIABLE *)
    csound->Calloc(csound, sizeof(CS_VARIABLE));
  IGN(p);
  var->memBlockSize = CS_FLOAT_ALIGN(sizeof(CS_OBJ));
  var->initializeVariableMemory = &csobj_var_init_memory;
  var->ctx = ctx;
  return var;
}

static int32_t create_csobj(CSOUND *csound, ASSIGN *p) {
  ((CS_OBJ *) p->r)->csound = csoundCreate(NULL, NULL);  
  return OK;
}

static int32_t setoption_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  STRINGDAT *code = (STRINGDAT *) p->b;
  CSOUND *engine = csobj->csound;
  *p->r = csoundSetOption(engine, code->data);
  return OK;
}

static int32_t compile_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  STRINGDAT *code = (STRINGDAT *) p->b;
  CSOUND *engine = csobj->csound;
  *p->r = csoundCompileOrc(engine, code->data, 0);
  return OK;
}

static int32_t start_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  CSOUND *engine = csobj->csound;
  uint32_t bsiz = (CS_KSMPS < engine->ksmps ?
                   engine->ksmps : CS_KSMPS)*sizeof(MYFLT);
  csobj->nsmps = engine->ksmps;
  
  csobj->bufferout = (MYFLT *) mcalloc(csound,bsiz*engine->nchnls);
  csobj->bufferin = (MYFLT *) mcalloc(csound,bsiz*engine->inchnls);  
  *p->r = csoundStart(engine);
  return OK;
}


static int32_t compilecsd_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  CSOUND *engine = csobj->csound;
  STRINGDAT *code = (STRINGDAT *) p->b;
  *p->r = csoundCompileCSD(engine, code->data, 0, 0);
  return OK;
}

static int32_t perform_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  CSOUND *engine = csobj->csound;
  uint32 ksmps = CS_KSMPS;
  uint32 esmps = engine->ksmps;
  int32_t nchnls = engine->nchnls, inc;
  int32_t inchnls = engine->inchnls;
  MYFLT *spout = engine->spout;
  MYFLT *bufferout = csobj->bufferout;
  MYFLT *spin = engine->spin;
  MYFLT *bufferin = csobj->bufferin;
  
  if(esmps >= ksmps) {
   for(int i=0; i < ksmps; i++) {
    if(csobj->nsmps == esmps) {
      *p->r = csoundPerformKsmps(engine);             
      csobj->nsmps = 0;
    }
    inc = csobj->nsmps*inchnls;
    memcpy(spin+inc,bufferin+inc,
            sizeof(MYFLT)*inchnls);
    inc = csobj->nsmps*nchnls;
    memcpy(bufferout+inc,
           spout+inc,
           sizeof(MYFLT)*nchnls);
    csobj->nsmps += 1;
   }
  } else {
    for(int j = 0; j < ksmps; j++) {
      if(csobj->nsmps == esmps) {
      *p->r = csoundPerformKsmps(engine);
       csobj->nsmps = 0;
      }
      inc = (csobj->nsmps*inchnls);
      memcpy(spin+inc,bufferin+j*nchnls,sizeof(MYFLT)*inchnls);
      inc = (csobj->nsmps*nchnls);
      memcpy(bufferout+j*nchnls,spout+inc,sizeof(MYFLT)*nchnls);      
      csobj->nsmps += 1;
    }
   }
  return OK;
}

static int32_t chnset_scalar_csobj(CSOUND *csound, AOP *p) {
    CS_OBJ *csobj = (CS_OBJ *) p->r;
    CSOUND *engine = csobj->csound;
    MYFLT  val = *p->a;
    STRINGDAT *channel = (STRINGDAT *) p->b;
    csoundSetControlChannel(engine, channel->data, val);
    return OK;
}

static int32_t chnset_vector_csobj(CSOUND *csound, AOP *p) {
    CS_OBJ *csobj = (CS_OBJ *) p->r;
    CSOUND *engine = csobj->csound;
    MYFLT  *val = p->a;
    STRINGDAT *channel = (STRINGDAT *) p->b;
    if(engine->ksmps == CS_KSMPS)
     csoundSetAudioChannel(engine, channel->data, val);
    else return csound->PerfError(csound,  &p->h,"ksmps do not match:\n"
                                  "csound obj (%d), instr (%d)\n",
                                  engine->ksmps, CS_KSMPS);
    return OK;
}

static int32_t chnget_scalar_csobj(CSOUND *csound, AOP *p) {
    CS_OBJ *csobj = (CS_OBJ *) p->a;
    CSOUND *engine = csobj->csound;
    int32_t  err; 
    STRINGDAT *channel = (STRINGDAT *) p->b;
    *p->r = csoundGetControlChannel(engine, channel->data, &err);
    if(err != CSOUND_SUCCESS) return NOTOK;
    return OK;
}

static int32_t chnget_vector_csobj(CSOUND *csound, AOP *p) {
    CS_OBJ *csobj = (CS_OBJ *) p->a;
    CSOUND *engine = csobj->csound;
    MYFLT  *val = p->r;
    STRINGDAT *channel = (STRINGDAT *) p->b;
    if(engine->ksmps == CS_KSMPS)
     csoundGetAudioChannel(engine, channel->data, val);
    else return csound->PerfError(csound,  &p->h, "ksmps do not match:\n"
                                  "csound obj (%d), instr (%d)\n",
                                  engine->ksmps, CS_KSMPS);
    return OK;
}


static int32_t getochn_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->a;
  CSOUND *engine = csobj->csound;
  int32_t chn = (int32_t) *p->b - 1;
  int32_t nchnls = engine->nchnls;
  uint32_t ksmps = CS_KSMPS;
  uint32_t esmps = engine->ksmps;
  MYFLT *out = p->r;
  MYFLT *in = csobj->bufferout;
   
  if(chn > engine->nchnls) {
    return csound->PerfError(csound, &p->h,
                             "requested channel %d not available\n",
                             chn);
  }

  if(esmps >= ksmps) {
    int start = csobj->nsmps-ksmps;
    if(start < 0) start += esmps;
    start *= nchnls;
    esmps *= nchnls;
    for(int i = start+chn, j = 0; j < ksmps; i+=nchnls, j++) 
      out[j] = in[i%esmps];
  } else {
    for(int i = chn, j = 0; j < ksmps; i+=nchnls, j++) 
      out[j] = in[i];
  }
  return OK;
}

static int32_t setichn_csobj(CSOUND *csound, AOP *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->r;
  CSOUND *engine = csobj->csound;
  int32_t chn = (int32_t) *p->a-1;
  int32_t nchnls = engine->inchnls;
  uint32_t ksmps = CS_KSMPS;
  uint32_t esmps = engine->ksmps;
  MYFLT *in = p->b;
  MYFLT *out = csobj->bufferin;  
  if(chn > engine->nchnls) {
    return csound->PerfError(csound, &p->h,
                             "requested channel %d not available\n",
                             chn+1);
  }
   
  if(esmps >= ksmps) {
    int start = csobj->nsmps;
    start *= nchnls;
    esmps *= nchnls;
    for(int i = start+chn, j = 0; j < ksmps; i+=nchnls, j++)
      out[i%esmps] = in[j];
  } else {
    for(int i = chn, j = 0; j < ksmps; i+=nchnls, j++)
      out[i] = in[j];
  }

  return OK;
}

static int32_t destroy_csobj(CSOUND *csound, ASSIGN *p) {
  CS_OBJ *csobj = (CS_OBJ *) p->r;
  mfree(csound, csobj->bufferout);
  mfree(csound, csobj->bufferin);  
  csoundDestroy(csobj->csound);
  csobj->csound = NULL;
  return OK;
}

const CS_TYPE CS_VAR_TYPE_CSOBJ = {
  "Csound", "Csound", CS_ARG_TYPE_BOTH,
  create_csobj_var, csobj_copy_value,
  NULL, NULL, 0
};

/** Add Csound type and opcodes
 */
void add_csobj(CSOUND *csound, TYPE_POOL *pool) {
  csoundAddVariableType(csound, pool,
                        (CS_TYPE*) &CS_VAR_TYPE_CSOBJ);
  csoundAppendOpcode(csound, "create", sizeof(ASSIGN), 0,
                       ":Csound;", "", (SUBR) create_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "setoption", sizeof(AOP), 0,
                       "i", ":Csound;S", (SUBR) setoption_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "compilestr", sizeof(AOP), 0,
                       "i", ":Csound;S", (SUBR) compile_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "compilecsd", sizeof(AOP), 0,
                       "i", ":Csound;S", (SUBR) compilecsd_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "start", sizeof(AOP), 0,
                       "i", ":Csound;", (SUBR) start_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "chnset", sizeof(AOP), 0,
                       "", ":Csound;iS", (SUBR) chnset_scalar_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "chnset", sizeof(AOP), 0,
                       "", ":Csound;kS", NULL, (SUBR) chnset_scalar_csobj, NULL);
  csoundAppendOpcode(csound, "chnget", sizeof(AOP), 0,
                       "i", ":Csound;S", (SUBR) chnget_scalar_csobj, NULL, NULL);
  csoundAppendOpcode(csound, "chnget", sizeof(AOP), 0,
                       "k", ":Csound;S", NULL, (SUBR) chnget_scalar_csobj, NULL);  
  csoundAppendOpcode(csound, "chnset", sizeof(AOP), 0,
                       "", ":Csound;aS", NULL, (SUBR) chnset_vector_csobj, NULL);
  csoundAppendOpcode(csound, "chnget", sizeof(AOP), 0,
                       "a", ":Csound;S", NULL, (SUBR) chnget_vector_csobj, NULL);
  csoundAppendOpcode(csound, "perf", sizeof(AOP), 0,
                       "k", ":Csound;", NULL, (SUBR) perform_csobj, NULL);
  csoundAppendOpcode(csound, "inch", sizeof(AOP), 0,
                       "a", ":Csound;k", NULL, (SUBR) getochn_csobj, NULL);
  csoundAppendOpcode(csound, "outch", sizeof(AOP), 0,
                       "", ":Csound;ka", NULL, (SUBR) setichn_csobj, NULL);
  csoundAppendOpcode(csound, "delete", sizeof(ASSIGN), 0,
                       "", ":Csound;", NULL, NULL, (SUBR) destroy_csobj);
  csoundAppendOpcode(csound, "destroy", sizeof(ASSIGN), 0,
                       "", ":Csound;", (SUBR) destroy_csobj, NULL, NULL);
  
}


