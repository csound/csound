#include "csdl.h"
#include "faust/llvm-dsp.h"

#define MAXARG 40


struct faustgen {
  OPDS h;
  MYFLT *outs[MAXARG]; /* outputs */
  STRINGDAT *code;     /* faust code as string */
  MYFLT *ins[VARGMAX]; /* inputs */
  llvm_dsp *engine;  /* faust DSP */
  llvm_dsp_factory* factory; /* DSP factory */
  AUXCH memin;
  AUXCH memout;
};

/* deinit function
   delete faust objects
*/
int delete_faustgen(CSOUND *csound, void *p) {
  faustgen *pp = (faustgen *) p;
  deleteDSPInstance(pp->engine);
  deleteDSPFactory(pp->factory);
  return OK;
}

/* init-time function
   compile faust program
*/
int init_faustgen(CSOUND *csound, faustgen *p){

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
                                "", "faustop", (const char *) p->code->data,
                                "", err_msg, 3);
  if(p->factory == NULL)
    return csound->InitError(csound,
                             Str("Faust compilation problem: %s\n"), err_msg);
  p->engine = createDSPInstance(p->factory);
  if(p->engine == NULL) {
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, Str("Faust instantiation problem \n"));
  }

  /* init engine */
  p->engine->init(csound->GetSr(csound));

  if(p->engine->getNumInputs() != p->INCOUNT-1) {
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, Str("wrong number of input args\n"));
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT){
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, Str("wrong number of output args\n"));
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
int perf_faustgen(CSOUND *csound, faustgen *p){
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

struct UI
{
  UI() {};
  virtual ~UI() {};
  virtual void openTabBox(const char* label) = 0;
  virtual void openHorizontalBox(const char* label) = 0;
  virtual void openVerticalBox(const char* label) = 0;
  virtual void closeBox() = 0;
  virtual void addButton(const char* label, FAUSTFLOAT* zone) = 0;
  virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) = 0;
  virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, 
   FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone,
    FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, 
    FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, 
                                      FAUSTFLOAT min, FAUSTFLOAT max) = 0;
  virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, 
                                      FAUSTFLOAT min, FAUSTFLOAT max) = 0;
  virtual void declare(FAUSTFLOAT* zone, const char* key, const char* val) {};
};

class controls : public UI {

  struct ctl {
    MYFLT *zone;
    char *label;
    MYFLT min, max;
    ctl  *nxt;
  } anchor;   

  void addctl(const char* label, FAUSTFLOAT* zone, 
	      FAUSTFLOAT min, FAUSTFLOAT max){
    ctl *pctl = &anchor;
    while(pctl->nxt) pctl = pctl->nxt;    
    pctl->nxt = new ctl;
    pctl->label = (char *) label;
    pctl->zone = zone;
    pctl->min = min;
    pctl->max = max;
  }

public:
 controls() { anchor.nxt = NULL; }
  ~controls() {
    ctl *pctl = &anchor, *tmp;
    while(pctl->nxt) {
      tmp = pctl;
      pctl = pctl->nxt;
      delete tmp;
  }
 }

 virtual void openTabBox(const char* label) {};
 virtual void openHorizontalBox(const char* label) {};
 virtual void openVerticalBox(const char* label) {};
 virtual void closeBox() {};

 virtual void addButton(const char* label, FAUSTFLOAT* zone) { 
    addctl(label, zone, 0, 0);
  }
  virtual void addCheckButton(const char* label, FAUSTFLOAT* zone){ 
    addctl(label, zone, 0, 0);
  }
  virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, 
   FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step){ 
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone,
    FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) { 
    addctl(label, zone, min, max);
  }
  virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, 
    FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) { 
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, 
				     FAUSTFLOAT min, FAUSTFLOAT max){};
  virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, 
				   FAUSTFLOAT min, FAUSTFLOAT max) {};

};


struct faustobj  {
  void *obj;
  controls *ctls;
  faustobj *nxt;
  int cnt;
};

struct faustcompile {
  OPDS h;
  MYFLT *hptr;
  STRINGDAT *code;
  STRINGDAT *args;
  llvm_dsp_factory *factory;
};

char **parse_cmd(char *str, int *argc){

  char **argv; int i = 0, n;
  while(str[i] == ' ') i++;
  if(str[i] != '\0') *argc = 1;
  while(str[i] != '\0'){
    if(str[i] == ' ') {
      while (str[++i] == ' ');  
      if(str[i] == '\0') break;
      (*argc)++;
    }
    i++;
  }
  argv = (char **) calloc(sizeof(char *), *argc);
  i = 0;
  while(str[i] == ' ') i++;
  for(n=0; n < *argc; n++) {
    argv[n] = &(str[i]);
    while(str[++i] != ' ');
    str[i] = '\0';
    while(str[++i] == ' ');
  }  
  return argv;

}

int delete_faustcompile(CSOUND *csound, void *p) {

  faustcompile *pp = (faustcompile *) p;
  faustobj *fobj, *prv;
  fobj = *((faustobj **) csound->QueryGlobalVariable(csound,"::factory"));
  prv = fobj;
  while(fobj != NULL) {
    if(fobj->obj == (void *) pp->factory) {
      prv->nxt = fobj->nxt;
      break;
    }
    prv = fobj;
    fobj = fobj->nxt;
  }
  if(fobj != NULL) {
  deleteDSPFactory(pp->factory);
  csound->Free(csound, fobj);
  }
 
  return OK;
}


int init_faustcompile(CSOUND *csound, faustcompile *p) {

  faustobj  **pffactory, *ffactory;
  llvm_dsp_factory *factory;
  int argc = 0;
  char *cmd = (char *) malloc(p->args->size + 8);
  strcpy(cmd, p->args->data);
#ifdef USE_DOUBLE
  strcat(cmd, " -double");
#endif
  const char **argv = (const char **) parse_cmd(cmd, &argc);
  const char* varname = "::factory";
  char err_msg[256];

  factory = createDSPFactory(argc, argv, "",
                                "", "faustop", (const char *) p->code->data,
                                "", err_msg, 3);
  
  if(factory == NULL) {
    free(argv);
    free(cmd);    
    return csound->InitError(csound,
                             Str("Faust compilation problem: %s\n"), err_msg);
   } 

  pffactory = (faustobj **) csound->QueryGlobalVariable(csound,varname);
  if(pffactory == NULL) {  
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pffactory = (faustobj **) csound->QueryGlobalVariable(csound,varname);
    ffactory = (faustobj *) csound->Calloc(csound, sizeof(faustobj)); 
    ffactory->obj = factory; 
    ffactory->nxt = NULL;
    ffactory->cnt = 0;
    *pffactory = ffactory;
  }
  else {
  ffactory = *pffactory;
  while(ffactory->nxt){
    ffactory = ffactory->nxt;
  }
  ffactory->nxt = (faustobj *) csound->Calloc(csound, sizeof(faustobj)); 
  ffactory->nxt->cnt = ffactory->cnt++;
  ffactory = ffactory->nxt;  
  ffactory->obj = factory;
  
  }
  p->factory = factory;
  *p->hptr = (MYFLT) ffactory->cnt;
  csound->RegisterDeinitCallback(csound, p, delete_faustcompile);
  free(argv);
  free(cmd);
  return OK;
}

struct faustinstance {
  OPDS h;
  MYFLT *ohptr;
  MYFLT *ihptr;
  llvm_dsp *dsp;
};

int delete_faustinstance(CSOUND *csound, void *p) {

  faustinstance *pp = (faustinstance *) p;
  faustobj *fobj, *prv;
  fobj = *((faustobj **) csound->QueryGlobalVariable(csound,"::dsp"));
  prv = fobj;
  while(fobj != NULL) {
    if(fobj->obj == (void *) pp->dsp) {
      prv->nxt = fobj->nxt;
      break;
    }
    prv = fobj;
    fobj = fobj->nxt;
  }
  if(fobj != NULL) {
  deleteDSPInstance(pp->dsp);
  csound->Free(csound, fobj);
  }
  
  return OK;
}

int init_faustinstance(CSOUND *csound, faustinstance *p){
     
    faustobj  *fobj, **pfdsp, *fdsp;
    llvm_dsp  *dsp;
    controls  *ctls;
    const char *varname = "::dsp";
    
    fobj = *((faustobj **) csound->QueryGlobalVariable(csound,"::factory"));
    if(fobj == NULL) 
      return csound->InitError(csound,
			       "no factory available\n");

    while(fobj->cnt != *p->ihptr) {
         fobj = fobj->nxt;
         if(fobj == NULL) 
             return csound->InitError(csound,
				      "factory not found %d\n", (int) *p->ihptr);
    }

   dsp = createDSPInstance((llvm_dsp_factory *)fobj->obj); 
   if(dsp == NULL) 
     return csound->InitError(csound, Str("Faust instantiation problem \n"));

   dsp->buildUserInterface(ctls);
     
    pfdsp = (faustobj **) csound->QueryGlobalVariable(csound,varname);
   if(pfdsp == NULL) {  
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **) csound->QueryGlobalVariable(csound,varname);
    fdsp = (faustobj *) csound->Calloc(csound, sizeof(faustobj)); 
    fdsp->obj = dsp; 
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  }
  else {
  fdsp = *pfdsp;
  while(fdsp->nxt){
    fdsp = fdsp->nxt;
  }
  fdsp->nxt = (faustobj *) csound->Calloc(csound, sizeof(faustobj)); 
  fdsp->nxt->cnt = fdsp->cnt++;
  fdsp = fdsp->nxt;  
  fdsp->obj = dsp;
  fdsp->ctls = ctls;
  }   
   csound->RegisterDeinitCallback(csound, p, delete_faustinstance); 
   *p->ohptr = (MYFLT) fdsp->cnt;
   return OK;
}

int init_faustaudio(CSOUND *csound, faustgen *p){

  faustobj *fobj;
  int instance = (int) *((MYFLT *)p->code);
  OPARMS parms;

  fobj = *((faustobj **) csound->QueryGlobalVariable(csound,"::dsp"));
    if(fobj == NULL) 
      return csound->InitError(csound,
			       "no dsp instances available\n");

    while(fobj->cnt != instance) {
         fobj = fobj->nxt;
         if(fobj == NULL) 
             return csound->InitError(csound,
				      "dsp instance not found %d\n", (int) *((MYFLT *)p->code));
    }

    p->engine = (llvm_dsp *) fobj->obj;  
    p->engine->init(csound->GetSr(csound));

  if(p->engine->getNumInputs() != p->INCOUNT-1) {
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, Str("wrong number of input args\n"));
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT){
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    return csound->InitError(csound, Str("wrong number of output args\n"));
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
  return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { (char *) "faustgen", S(faustgen), 0, 5,
    (char *) "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    (char *)"Sy",(SUBR)init_faustgen, NULL, (SUBR)perf_faustgen},
 { (char *) "faustcompile", S(faustcompile), 0, 1,
    (char *) "i",
   (char *)"SS",(SUBR)init_faustcompile, NULL, NULL},
 { (char *) "faustinstance", S(faustinstance), 0, 1,
    (char *) "i",
   (char *)"i",(SUBR)init_faustinstance, NULL, NULL},
 { (char *) "faustaudio", S(faustgen), 0, 5,
    (char *) "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
   (char *)"iy",(SUBR)init_faustaudio, NULL, (SUBR)perf_faustgen}
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
