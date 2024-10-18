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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "compile_ops.h"
#include <stdio.h>
int32_t csoundCompileOrcInternal(CSOUND *csound, const char *str, int32_t async);
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
  *p->res = (MYFLT)(csoundCompileOrcInternal(csound, orc, 0));
  fclose(fp);
  csound->Free(csound,orc);
  return OK;
}

int32_t compile_csd_i(CSOUND *csound, COMPILE *p){
  *p->res = (MYFLT) csoundCompileCsd(csound, ((STRINGDAT *)p->str)->data);
  return OK;
}

int32_t compile_str_i(CSOUND *csound, COMPILE *p){
  //void csp_orc_sa_print_list(CSOUND*);
  //printf("START\n");
  *p->res = (MYFLT)(csoundCompileOrcInternal(csound,
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
  char *code;
  const char *endin = "\n endin \n";
  // look for a free slot
  while(instrs[num] != NULL) num++;
    code = csound->Calloc(csound, strlen(p->code->data)
                               + strlen(endin) + 16);
   sprintf(code, "instr %d\n%s%s", num, p->code->data, endin);

  if(csound->GetDebug(csound)) csound->Message(csound, "%s \n", code);
  // compile code
  if(csoundCompileOrcInternal(csound, code, 0) == CSOUND_SUCCESS) {
    // pass the instrument out
    p->instr->instr = instrs[num];
    csound->Free(csound, code);
    return OK;
  }
  csound->Free(csound, code);
  return csound->InitError(csound, "failed to compile instr\n");
}
#include "linevent.h"
int32_t eventOpcodeI_(CSOUND *csound, LINEVENT *p, int32_t s, char p1);
/* compiles and runs an anonymous instrument
   run_instr Scode, idur[, ...]  
*/
int32_t compile_and_run_instr(CSOUND *csound, CARINSTR *p) {
  char *code;
  const char *endin = "\n endin \n";
  const char *name = "instr __ANONYMOUS__ \n";
  code = csound->Calloc(csound, strlen(p->code->data)
                        + strlen(name) + strlen(endin) + 1);
  sprintf(code, "%s%s%s", name, p->code->data, endin);
  if(csound->GetDebug(csound))
    csound->Message(csound, "%s \n", code);
  // compile code
  if(csoundCompileOrcInternal(csound, code, 0) == CSOUND_SUCCESS) {
       LINEVENT pp;
       MYFLT zero = FL(0.0);
       MYFLT num = (MYFLT) csound->StringArg2Insno(csound, "__ANONYMOUS__", 1);
       int32_t i;
       pp.h = p->h;
       char c[2] = "i";
       pp.args[0] = (MYFLT *) c;
       pp.args[1] = (MYFLT *) &num;
       pp.args[2] = &zero;
       pp.argno = p->INOCOUNT+2;
       for (i=0; i < p->INOCOUNT-1;i++) {
         pp.args[i+3] = p->argums[i];
       }
      pp.flag = 1;
      csound->Free(csound, code); 
      return eventOpcodeI_(csound, &pp, 0, 'i');
  }
  csound->Free(csound, code);
  return csound->InitError(csound, "failed to compile instr\n");
}

int32_t run_instr(CSOUND *csound, RINSTR *p) {
       LINEVENT pp;
       MYFLT zero = FL(0.0);
       int32_t i;
       pp.h = p->h;
       char c[2] = "i";
       pp.args[0] = (MYFLT *) c;
       pp.args[1] = (MYFLT *) p->instr;
       pp.args[2] = &zero;
       pp.argno = p->INOCOUNT+2;
       for (i=0; i < p->INOCOUNT-1;i++) {
         pp.args[i+3] = p->argums[i];
       }
      pp.flag = 1;
      return eventOpcodeI_(csound, &pp, 2, 'i');
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
    if(!strcmp(p->address, address) && !strcmp(p->type, type) &&
       p->flag) break;
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
  tabinit(csound, p->out, (int32_t) strlen(p->type->data), &(p->h));
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
