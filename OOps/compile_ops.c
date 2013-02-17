
#include "compile_ops.h"
#include <stdio.h>

int compile_orc_i(CSOUND *csound, COMPILE *p){
  FILE *fp;
  int size=0;
  char *orc, c, *name = csound->strarg2name(csound,NULL,p->str,"",1);

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
  *p->res = FL(csoundCompileOrc(csound, (char *)p->str));
  return OK;
}
