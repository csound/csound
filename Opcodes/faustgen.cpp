#include "csdl.h"
#include "faust/llvm-dsp.h"

#define MAXARG 32

typedef struct {
  OPDS h;
  MYFLT *outs[MAXARG]; /* outputs */
  STRINGDAT *code;     /* faust code as string */
  MYFLT *ins[MAXARG]; /* inputs */
  dsp *engine;  /* faust DSP */
  llvm_dsp_factory* factory; /* DSP factory */
} FAUSTGEN;

/* deinit function
   delete faust objects 
*/
int delete_faustgen(CSOUND *csound, void *p) {
  FAUSTGEN *pp = (FAUSTGEN *) p;
  deleteDSPInstance((llvm_dsp *)pp->engine);
  deleteDSPFactory(pp->factory);
  return OK;
}

/* init-time function 
   compile faust program
*/
int init_faustgen(CSOUND *csound, FAUSTGEN *p){

  char err_msg[256];
  int size;
  int argc = 3;
  const char* argv[argc];
  argv[0] = "-vec";
  argv[1] = "-lv";
  argv[2] = " 1";

#ifdef USE_DOUBLE
  argv[3] = "-double";
  argc += 1;
#endif

  /* compile engine 
     p->code->data holds the faust program
  */
  p->factory = createDSPFactory(argc, argv, "",
				"", "faustop", (const char *) p->code->data,"", err_msg, 3);
  if(p->factory == NULL) 
    return csound->InitError(csound, "Faust compilation problem: %s\n", err_msg);
  p->engine = createDSPInstance(p->factory);
  if(p->engine == NULL) 
    return csound->InitError(csound, "Faust instantiation problem \n");
  
  /* init engine */
  p->engine->init(csound->GetSr(csound));

  if(p->engine->getNumInputs() != p->INCOUNT-1) {
   deleteDSPInstance((llvm_dsp *) p->engine);
   deleteDSPFactory(p->factory);
   return csound->InitError(csound, "wrong number of input args\n");
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT){
    deleteDSPInstance((llvm_dsp *)p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, "wrong number of output args\n");
  }
 
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  return OK;
}

/* perf-time function
   FIXME: need to implement cs6 sample-accurate support
   TODO:  k-rate controls
*/
int perf_faustgen(CSOUND *csound, FAUSTGEN *p){

  int nsmps = CS_KSMPS, i, j;
  int faustins = p->engine->getNumInputs();   
  int faustouts = p->engine->getNumOutputs();  
  MYFLT **ins, **outs;
  
  for(i=0; i < faustins; i++) ins[i] = p->ins[i];
  for(i=0; i < faustouts; i++)  outs[i] = p->outs[i];
    
  p->engine->compute(nsmps, ins, outs);

  return OK;
}
