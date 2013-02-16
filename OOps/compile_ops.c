
#include "compile_ops.h"
#include <stdio.h>

int compile_orc_i(CSOUND *csound, COMPILE *p){
  FILE *fp;
  int size=0;
  char *orc, c;
  fp = fopen(csound->strarg2name(csound,NULL,p->str,"",1), "rb"); 
  
  while(!feof(fp)) 
     size += fread(&c,1,1,fp);

  orc = (char *) mcalloc(csound, size+1);
  fseek(fp, 0, SEEK_SET);    
  fread(orc,1,size,fp);
  /* we need to notify the lock so that merge can occur 
     this might possibly compromise thread-safety
  */
  csoundNotifyThreadLock(csound->API_lock);
  csoundCompileOrc(csound, orc);
  fclose(fp);
  mfree(csound,orc);
  return OK;
}

int compile_str_i(CSOUND *csound, COMPILE *p){
  /* we need to notify the lock so that merge can occur 
     this might possibly compromise thread-safety
  */
  csoundNotifyThreadLock(csound->API_lock);
  csoundCompileOrc(csound, (char *)p->str);
  return OK;
}
