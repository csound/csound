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
         Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
         02111-1307 USA
*/

#include "compile_ops.h"
#include <stdio.h>

int compile_orc_i(CSOUND *csound, COMPILE *p){
  FILE *fp;
  int size=0;
  char *orc, c, *name;


  name = ((STRINGDAT *)p->str)->data;
  fp = fopen(name, "rb");

  if(fp == NULL) {
    csound->Warning(csound, Str("compileorc: could not open %s\n"), name);
    *p->res = FL(CSOUND_ERROR);
    return NOTOK;
  }

  while(!feof(fp))
     size += fread(&c,1,1,fp);

  if(size==0) {
   fclose(fp);
   *p->res = FL(CSOUND_ERROR);
   csound->InitError(csound, Str("compileorc: could not read %s\n"), name);
   return NOTOK;
  }

  orc = (char *) mcalloc(csound, size+1);
  fseek(fp, 0, SEEK_SET);
  fread(orc,1,size,fp);
  *p->res = FL(csoundCompileOrc(csound, orc));
  fclose(fp);
  mfree(csound,orc);
  return OK;
}

int compile_str_i(CSOUND *csound, COMPILE *p){
  *p->res = FL(csoundCompileOrc(csound, ((STRINGDAT *)p->str)->data));
  return OK;
}

int read_score_i(CSOUND *csound, COMPILE *p){
  *p->res = FL(csoundReadScore(csound, ((STRINGDAT *)p->str)->data));
  return OK;
}

int eval_str_i(CSOUND *csound, COMPILE *p){
  *p->res = csoundEvalCode(csound, ((STRINGDAT *)p->str)->data);
  return OK;
}

int eval_str_k(CSOUND *csound, COMPILE *p){
  if(*p->ktrig)
   *p->res = csoundEvalCode(csound, ((STRINGDAT *)p->str)->data);
  return OK;
}


int retval_i(CSOUND *csound, RETVAL *p){
  INSDS *ip = p->h.insdshead;
  ip->retval = *p->ret;
  return OK;
}

