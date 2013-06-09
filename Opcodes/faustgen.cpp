#include "csdl.h"
#include "faust/llvm-dsp.h"

#define MAXARG 40

typedef struct {
  OPDS h;
  MYFLT *outs[MAXARG]; /* outputs */
  STRINGDAT *code;     /* faust code as string */
  MYFLT *ins[VARGMAX]; /* inputs */
  llvm_dsp *engine;  /* faust DSP */
  llvm_dsp_factory* factory; /* DSP factory */
  AUXCH memin;
  AUXCH memout;
} FAUSTGEN;

/* deinit function
   delete faust objects 
*/
int delete_faustgen(CSOUND *csound, void *p) {
  FAUSTGEN *pp = (FAUSTGEN *) p;
  deleteDSPInstance(pp->engine);
  deleteDSPFactory(pp->factory);
  return OK;
}

/* init-time function 
   compile faust program
*/
int init_faustgen(CSOUND *csound, FAUSTGEN *p){

  OPARMS parms;
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
  if(p->engine == NULL) {
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, "Faust instantiation problem \n");
  }
  
  /* init engine */
  p->engine->init(csound->GetSr(csound));

  if(p->engine->getNumInputs() != p->INCOUNT-1) {
   deleteDSPInstance(p->engine);
   deleteDSPFactory(p->factory);
   return csound->InitError(csound, "wrong number of input args\n");
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT){
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, "wrong number of output args\n");
  }
  /* memory for sampAccurate offsets */
  csound->GetOParms(csound, &parms);
  if(parms.sampleAccurate){
    int size;
    size = p->engine->getNumInputs()*sizeof(MYFLT *);
    if(p->memin.auxp == NULL ||
       p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs()*sizeof(MYFLT *);
    if(p->memout.auxp == NULL ||
       p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }

  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  return OK;
}

/* perf-time function
   TODO:  k-rate controls
*/
int perf_faustgen(CSOUND *csound, FAUSTGEN *p){
  int nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  MYFLT **in_tmp = (MYFLT **) p->memin.auxp;
  MYFLT **out_tmp = (MYFLT **) p->memout.auxp;

  if (UNLIKELY(early)) {
      for (i = 0; i < p->OUTCOUNT; i++)
        memset(p->outs[i], '\0', nsmps*sizeof(MYFLT));
      nsmps -= early;
   }
  if(UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTCOUNT; i++){ 
        memset(p->outs[i], '\0', nsmps*sizeof(MYFLT));
        out_tmp[i] = p->outs[i];
        p->outs[i] = &(p->outs[i][offset]);
  }
    for (i = 0; i < p->INCOUNT-1; i++){ 
        in_tmp[i] = p->ins[i];
        p->ins[i] = &(p->ins[i][offset]);
  }
    nsmps -= offset;
 }
  p->engine->compute(nsmps, p->ins, p->outs);

  if(UNLIKELY(offset)) {
    /* restore pos  */
    for (i = 0; i < p->OUTCOUNT; i++)
	p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INCOUNT-1; i++)
        p->ins[i] = in_tmp[i];
  }

  return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { (char *) "faustgen", S(FAUSTGEN), 0, 5, (char *) "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", 
     (char *)"Sy",(SUBR)init_faustgen, NULL, (SUBR)perf_faustgen}
};

PUBLIC long csound_opcode_init(CSOUND *csound, OENTRY **ep)
{
    IGN(csound);
    *ep = localops;
    return (long) sizeof(localops);
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
