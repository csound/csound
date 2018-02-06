/*  compile_ops.c:

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

