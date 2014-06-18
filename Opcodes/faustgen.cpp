/*  faustgen.cpp

    (c) Victor Lazzarini, 2013

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

    Faust Csound opcodes

    These four opcodes allow Faust to be embedded in Csound code:

    faustcompile: compiles a Faust program
    faustaudio: creates a DSP instance from a compiled Faust program
    (any number of instances from a single compiled program are
    allowed)
    faustctl: sets the value of a given control of a Faust DSP instance

    faustgen: compiles and creates a single DSP instance from a Faust program
    (convenient for one-off Faust programs)


*/
#include "csdl.h"
#include "faust/llvm-dsp.h"
#include "faust/gui/UI.h"
#if defined(MACOSX) || defined(linux) || defined(HAIKU)
#include <unistd.h>
#endif

#define MAXARG 40

/**
 * Faust controls class for Csound
 *
 **/
class controls : public UI {

  struct ctl {
    MYFLT *zone;
    char label[64];
    MYFLT min, max;
    ctl  *nxt;
  } anchor;

  void addctl(const char* label, FAUSTFLOAT* zone,
              FAUSTFLOAT min, FAUSTFLOAT max){
    ctl *pctl = &anchor;
    while(pctl->nxt) pctl = pctl->nxt;
    pctl->nxt = new ctl;
    pctl = pctl->nxt;
    strncpy(pctl->label,label, 63);
    pctl->zone = zone;
    pctl->min = min;
    pctl->max = max;
    pctl->nxt = NULL;
  }

public:
  controls() { anchor.nxt = NULL; anchor.label[0] =  '\0';}
  ~controls() {
    ctl *pctl = &anchor, *tmp;
    pctl = pctl->nxt;
    while(pctl) {
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
                                 FAUSTFLOAT init, FAUSTFLOAT min,
                                 FAUSTFLOAT max, FAUSTFLOAT step){
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min,
                                   FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addNumEntry(const char* label, FAUSTFLOAT* zone,
                           FAUSTFLOAT init, FAUSTFLOAT min,
                           FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone,
                                     FAUSTFLOAT min, FAUSTFLOAT max){};
  virtual void addVerticalBargraph(const char* label, FAUSTFLOAT* zone,
                                   FAUSTFLOAT min, FAUSTFLOAT max) {};

  MYFLT *getZone(char *label){
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while(pctl){
      if(strcmp(pctl->label, label) == 0) break;
      pctl = pctl->nxt;
    }
    if(pctl)
      return pctl->zone;
    else return NULL;
  }
  MYFLT getMax(char *label){
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while(pctl){
      if(strcmp(pctl->label, label) == 0) break;
      pctl = pctl->nxt;
    }
    if(pctl)
      return pctl->max;
    else return 0;
  }
  MYFLT getMin(char *label){
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while(pctl){
      if(strcmp(pctl->label, label) == 0) break;
      pctl = pctl->nxt;
    }
    if(pctl)
      return pctl->min;
    else return 0;
  }

};

/**
 * Faust object handle
 *
 **/
struct faustobj  {
  void *obj;
  controls *ctls;
  faustobj *nxt;
  unsigned long cnt;
};

/**
 * Faust compile opcode

 usage:
 ihandle  faustcompile Scode, Sargs[,istacksize]

 ihandle - handle to compiled code
 Scode - Faust program
 Sargs - Faust compiler args
 istacksize - compiler stack size in megabytes (default 1MB).

**/
struct faustcompile {
  OPDS h;
  MYFLT *hptr;
  STRINGDAT *code;
  STRINGDAT *args;
  MYFLT     *stacksize;
  llvm_dsp_factory *factory;
  pthread_mutex_t *lock;
};

char **parse_cmd(char *str, int *argc){
  char **argv; int i = 0, n=0, end = strlen(str);
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
  for(n=0; n < *argc && i < end; n++) {
    argv[n] = &(str[i]);
    while(str[++i] != ' ' && i < end);
    if(i >= end) break;
    str[i] = '\0';
    while(str[++i] == ' ' && i < end);
  }
  return argv;
}

int delete_faustcompile(CSOUND *csound, void *p) {

  faustcompile *pp = ((faustcompile *) p);
  faustobj *fobj, *prv, **pfobj;
  pfobj = (faustobj **) csound->QueryGlobalVariable(csound,"::factory");
  fobj = *pfobj;
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
    if(*pfobj == fobj) *pfobj = fobj->nxt;
    deleteDSPFactory(pp->factory);
    csound->Free(csound, fobj);
  }
  return OK;
}

struct hdata {
  faustcompile *p;
  CSOUND *csound;
};

void *init_faustcompile_thread(void *pp) {

  faustcompile *p = ((hdata *) pp)->p;
  faustobj  **pffactory, *ffactory;
  CSOUND *csound = ((hdata *) pp)->csound;
  llvm_dsp_factory *factory;
  int argc = 0;
  std::string err_msg;
  char *cmd = (char *) malloc(p->args->size + 8);
  int ret;
 
  strcpy(cmd, p->args->data);
#ifdef USE_DOUBLE
  strcat(cmd, " -double");
#endif
  const char **argv = (const char **) parse_cmd(cmd, &argc);
  const char* varname = "::factory";

  //Need to protect this 

  csound->LockMutex(p->lock);
  //csound->Message(csound, "lock %p\n", p->lock);
  factory = createDSPFactoryFromString("faustop",
                                       (const char *) p->code->data,argc, argv,
                             "", err_msg, 3);
  //csound->Message(csound, "unlock %p\n", p->lock);
  csound->UnlockMutex(p->lock);

  if(factory == NULL) {
    csound->Message(csound,
                    Str("\nFaust compilation problem:\nline %s\n"),
                    err_msg.c_str());
    *(p->hptr) = FL(-2.0); // error code.
    free(argv);
    free(cmd);
    free(pp);
    ret = -1;
    pthread_exit(&ret);
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
    ffactory->nxt->cnt = ffactory->cnt+1;
    ffactory = ffactory->nxt;
    ffactory->obj = factory;
  }
  p->factory = factory;
  *p->hptr = (MYFLT) ffactory->cnt;
  csound->RegisterResetCallback(csound, p, delete_faustcompile);
  free(argv);
  free(cmd);
  free(pp);

  return NULL;
}

#define MBYTE 1048576
int init_faustcompile(CSOUND *csound, faustcompile *p){
  pthread_t thread;
  pthread_attr_t attr;
  hdata *data = (hdata *) malloc(sizeof(hdata));
  data->csound = csound;
  data->p = p;
  *p->hptr = -1;

  p->lock = (pthread_mutex_t *) csound->QueryGlobalVariable(csound,"::faustlock::");
  if(p->lock == NULL) {
    csound->CreateGlobalVariable(csound,"::faustlock::", sizeof(pthread_mutex_t));
    p->lock = (pthread_mutex_t *) csound->QueryGlobalVariable(csound,"::faustlock::");
    pthread_mutex_init(p->lock, NULL);
    //csound->Message(csound, "lock created %p\n", p->lock);
  }

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, *p->stacksize*MBYTE);
  pthread_create(&thread, &attr,init_faustcompile_thread, data);
  return OK;
}

/**
 * faustgen and faustaudio opcodes

 usage:
 ihandle[,asig1,...] faustgen    Scode[,ain1,...]
 ihandle[,asig1,...] faustaudio  ifactory,[,ain1,...]

 Scode - Faust program
 ifactory - handle pointing to compiled code from faustcompile
 asig1 ... - audio outputs from Faust program
 ain1 ...  - audio inputs to Faust program

 ihandle - handle identifying this Faust DSP instance

**/
struct faustgen {
  OPDS h;
  MYFLT *ohptr;
  MYFLT *outs[MAXARG]; /* outputs */
  STRINGDAT *code;     /* faust code as string */
  MYFLT *ins[VARGMAX]; /* inputs */
  llvm_dsp *engine;  /* faust DSP */
  llvm_dsp_factory* factory; /* DSP factory */
  controls *ctls;
  AUXCH memin;
  AUXCH memout;
};

struct hdata2 {
  faustgen *p;
  CSOUND *csound;
};

/* deinit function
   delete faust objects
*/
int delete_faustgen(CSOUND *csound, void *p) {
  faustgen *pp = (faustgen *) p;
  faustobj *fobj, *prv, **pfobj;
  pfobj = (faustobj **) csound->QueryGlobalVariable(csound,"::dsp");
  fobj = *pfobj;
  prv = fobj;
  while(fobj != NULL) {
    if(fobj->obj == (void *) pp->engine) {
      prv->nxt = fobj->nxt;
      break;
    }
    prv = fobj;
    fobj = fobj->nxt;
  }
    if(fobj != NULL) {
      if(*pfobj == fobj) *pfobj = fobj->nxt;
      csound->Free(csound, fobj);
      delete pp->ctls;
      deleteDSPInstance(pp->engine);
    } else
   csound->Warning(csound,
                      Str("could not find DSP %p for deletion"), pp->engine);
    if(pp->factory) deleteDSPFactory(pp->factory);

  return OK;
}



int init_faustaudio(CSOUND *csound, faustgen *p){
  int factory;
  OPARMS parms;
  faustobj  *fobj, **fobjp, **pfdsp, *fdsp;
  llvm_dsp  *dsp;
  controls  *ctls = new controls();
  const char *varname = "::dsp";
#if defined(MACOSX) || defined(linux) || defined(HAIKU)
  while((int) *((MYFLT *)p->code) == -1) usleep(1);
#else
  while((int) *((MYFLT *)p->code) == -1) Sleep(1);
#endif

  factory = (int) *((MYFLT *)p->code);

  if(factory == -2)
    return
      csound->InitError(csound,
                        Str("Faust code did not compile properly.\n"
                            "Check above messages for Faust compiler errors\n"));

  fobjp = (faustobj **) csound->QueryGlobalVariable(csound,"::factory");
  if(fobjp == NULL)
    return csound->InitError(csound,
                             Str("no factory available\n"));
  fobj = *fobjp;
  while(fobj->cnt != factory) {
    fobj = fobj->nxt;
    if(fobj == NULL)
      return csound->InitError(csound,
                               Str("factory not found %d\n"), (int) factory);
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
    if(fdsp != NULL){
    while(fdsp->nxt){
      fdsp = fdsp->nxt;
    }
    fdsp->nxt = (faustobj *) csound->Calloc(csound, sizeof(faustobj));
    fdsp->nxt->cnt = fdsp->cnt+1;
    fdsp = fdsp->nxt;
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    }
    else {
    fdsp = (faustobj *) csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
    } 
  }

  p->factory = NULL;  // this opcode does not own the factory
  p->engine = (llvm_dsp *) fdsp->obj;
  p->engine->init(csound->GetSr(csound));

  if(p->engine->getNumInputs() != p->INCOUNT-1) {
    deleteDSPInstance(p->engine);
    return csound->InitError(csound, Str("wrong number of input args\n"));
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT-1){
    deleteDSPInstance(p->engine);
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
  p->ctls = ctls;
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  *p->ohptr = (MYFLT) fdsp->cnt;
  return OK;
}

void *init_faustgen_thread(void *pp){
  CSOUND *csound = ((hdata2 *) pp)->csound;
  faustgen *p = ((hdata2 *) pp)->p;
  OPARMS parms;
  std::string err_msg;
  int size;
  int argc = 3;
  const char* argv[argc];
  faustobj  **pfdsp, *fdsp;
  llvm_dsp  *dsp;
  controls  *ctls = new controls();
  const char *varname = "::dsp";
  argv[0] = "-vec";
  argv[1] = "-lv";
  argv[2] = " 1";
  p->engine = NULL;

#ifdef USE_DOUBLE
  argv[3] = "-double";
  argc += 1;
#endif

  p->factory = createDSPFactoryFromString("faustop",
                                          (const char *) p->code->data, argc, argv,
                                "", err_msg, 3);
  if(p->factory == NULL) {
    int ret = csound->InitError(csound,
                                Str("Faust compilation problem: %s\n"),
                                err_msg.c_str());
    free(pp);
    pthread_exit(&ret);
  }

  dsp = createDSPInstance(p->factory);
  if(dsp == NULL) {
    int ret = csound->InitError(csound, Str("Faust instantiation problem \n"));
    free(pp);
    pthread_exit(&ret);
  }

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

  p->engine = dsp;
  dsp->buildUserInterface(ctls);
  dsp->init(csound->GetSr(csound));
  if(p->engine->getNumInputs() != p->INCOUNT-1) {
    int ret;
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    free(pp);
    ret  =csound->InitError(csound, Str("wrong number of input args\n"));
    p->engine = NULL;
    pthread_exit(&ret);
  }
  if(p->engine->getNumOutputs() != p->OUTCOUNT-1){
    int ret;
    deleteDSPInstance(p->engine);
    deleteDSPFactory(p->factory);
    free(pp);
    ret = csound->InitError(csound, Str("wrong number of output args\n"));
    p->engine = NULL;
    pthread_exit(&ret);
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
  p->ctls = ctls;
  *p->ohptr = (MYFLT) fdsp->cnt;
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  free(pp);
  return NULL;
}

int init_faustgen(CSOUND *csound, faustgen *p){
  pthread_t thread;
  pthread_attr_t attr;
  int *ret;
  hdata2 *data = (hdata2 *) malloc(sizeof(hdata2));
  data->csound = csound;
  data->p = p;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, MBYTE);
  pthread_create(&thread, &attr, init_faustgen_thread, data);
  pthread_join(thread, (void **) &ret);
  if(ret == NULL) return OK;
  else return NOTOK;
}

int perf_faust(CSOUND *csound, faustgen *p){
  int nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  MYFLT **in_tmp = (MYFLT **) p->memin.auxp;
  MYFLT **out_tmp = (MYFLT **) p->memout.auxp;
  AVOIDDENORMALS;

  if (UNLIKELY(early)) {
    for (i = 0; i < p->OUTCOUNT-1; i++)
      memset(p->outs[i], '\0', nsmps*sizeof(MYFLT));
    nsmps -= early;
  }
  if(UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTCOUNT-1; i++){
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
    for (i = 0; i < p->OUTCOUNT-1; i++)
      p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INCOUNT-1; i++)
      p->ins[i] = in_tmp[i];
  }
  return OK;
}

/**
 * faustctl opcode

 usage:
 faustctl  idsp, Slabel, kval

 idsp - handle from an existing Faust DSP instance
 Slabel - name of control (in Faust program)
 kval - value to be sent to control

**/
struct faustctl {
  OPDS h;
  MYFLT *inst;
  STRINGDAT *label;
  MYFLT *val;
  MYFLT *zone;
  MYFLT min, max;
};

int init_faustctl(CSOUND *csound, faustctl *p){

  faustobj *fobj, **fobjp;
  int instance = (int) *p->inst;

  fobjp = (faustobj **) csound->QueryGlobalVariable(csound,"::dsp");
  if(fobjp == NULL)
    return csound->InitError(csound,
                             Str("no dsp instances available\n"));
  fobj = *fobjp;

  while(fobj->cnt != instance) {
    fobj = fobj->nxt;
    if(fobj == NULL)
      return csound->InitError(csound,
                               Str("dsp instance not found %d\n"), (int) *p->inst);
  }
  p->zone = fobj->ctls->getZone(p->label->data);
  if(p->zone == NULL)
    return csound->InitError(csound,
                             Str("dsp control %s not found\n"), p->label->data);
  p->max = fobj->ctls->getMax(p->label->data);
  p->min = fobj->ctls->getMin(p->label->data);
  {
   MYFLT val = *p->val;
   if(p->min != p->max)
    val = val < p->min ? p->min : (val > p->max ? p->max : val);
   *p->zone = val;
  }
  return OK;
}

int perf_faustctl(CSOUND *csound, faustctl *p) {
  MYFLT val = *p->val;
  if(p->min != p->max)
    val = val < p->min ? p->min : (val > p->max ? p->max : val);
  *p->zone = val;
  return OK;
}


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { (char *) "faustgen", S(faustgen), 0, 5,
    (char *) "immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    (char *)"SM",(SUBR)init_faustgen, NULL, (SUBR)perf_faust},
  { (char *) "faustcompile", S(faustcompile), 0, 1,
    (char *) "i",
    (char *)"SSp",(SUBR)init_faustcompile, NULL, NULL},
  { (char *) "faustaudio", S(faustgen), 0, 5,
    (char *) "immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    (char *)"iM",(SUBR)init_faustaudio, NULL, (SUBR)perf_faust},
  { (char *) "faustctl", S(faustgen), 0, 3,
    (char *) "",
    (char *)"iSk",(SUBR)init_faustctl, (SUBR) perf_faustctl}
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
