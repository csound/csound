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
  // no messages, just exit
  if(p->address == NULL) return NULL;
  do {
    if(!strcmp(p->address, address) && !strcmp(p->type, type) &&
       p->flag) return p;
  } while((p = p->nxt) != NULL);
  return NULL;
}

/** Clear flag for OSC message so its slot can be reused.
 */
void csoundClearOSCMessage(OSC_MESS *mess){
  mess->flag = 0;
}

/** Get float from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetMYFLT(const char *buf, MYFLT *mf) {
  float f;
  f = *((float *) buf);
  byteswap((char*)&f,4);
  *mf = f;
  return buf + 4;
}

/** Get int from Osc Message data 
    returns pointer to next datum
*/
const char *csoundOSCMessageGetInt(const char *buf, int32_t *i) {
  *i = *((int32_t *) buf);
  byteswap((char*)i,4);
  return buf + 4;
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

int32_t readOSC_perf(CSOUND *csound, ROSC *p) {
  int cnt = p->OUTOCOUNT - 1, i;
  if(cnt > 32)
    return csound->PerfError(csound, &(p->h),
                             "OSCRead exceeded max output args (>32)\n");
  MYFLT **out = p->out;
  OSC_MESS *mess = csoundReadOSCMessage(csound, p->address->data, p->type->data);
  if(mess != NULL) {
    const char *buf = mess->data;
    for(i = 0; i < cnt; i++) {
      if(p->type->data[i] == 's' &&
           IS_STR_ARG(out[i])) {
        buf = csoundOSCMessageGetString(buf, (STRINGDAT *) out[i]);
       }
      else if(p->type->data[i] == 'f' &&
                IS_KSIG_ARG(p->out[i])) {
         buf = csoundOSCMessageGetMYFLT(buf,p->out[i]);
       }
      else if(p->type->data[i] == 'i' &&
              IS_KSIG_ARG(out[i])) {
        int32_t d;
        buf = csoundOSCMessageGetInt(buf, &d);
        *(out[i]) = (MYFLT) d;
      }
      else csound->PerfError(csound, &(p->h), "OSC type mismatch\n");
    }
    *p->kstatus = 1;
    csoundClearOSCMessage(mess);
  }

  return OK;
}
