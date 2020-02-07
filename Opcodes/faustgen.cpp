/*  faustgen.cpp

    Copyright (c) Victor Lazzarini, 2013

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
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/UI.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <string>
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
    FAUSTFLOAT *zone;
    char label[65];
    MYFLT min, max;
    ctl *nxt;
  } anchor;

  void addctl(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT min,
              FAUSTFLOAT max) {
    ctl *pctl = &anchor;
    while (pctl->nxt)
      pctl = pctl->nxt;
    pctl->nxt = new ctl;
    pctl = pctl->nxt;
    strncpy(pctl->label, label, 63);
    pctl->label[63] = '\n'; pctl->label[64] = '\0';
    pctl->zone = zone;
    pctl->min = min;
    pctl->max = max;
    pctl->nxt = NULL;
  }

public:
  controls() {
    anchor.nxt = NULL;
    anchor.label[0] = '\0';
  }
  ~controls() {
    ctl *pctl = &anchor, *tmp;
    pctl = pctl->nxt;
    while (pctl) {
      tmp = pctl;
      pctl = pctl->nxt;
      delete tmp;
    }
  }

  virtual void openTabBox(const char *label){};
  virtual void openHorizontalBox(const char *label){};
  virtual void openVerticalBox(const char *label){};
  virtual void closeBox(){};

  virtual void addSoundfile(const char *label, const char *filename,
                            Soundfile **sf_zone){};

  virtual void addButton(const char *label, FAUSTFLOAT *zone) {
    addctl(label, zone, 0, 0);
  }
  virtual void addCheckButton(const char *label, FAUSTFLOAT *zone) {
    addctl(label, zone, 0, 0);
  }
  virtual void addVerticalSlider(const char *label, FAUSTFLOAT *zone,
                                 FAUSTFLOAT init, FAUSTFLOAT min,
                                 FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalSlider(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min,
                                   FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init,
                           FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
    addctl(label, zone, min, max);
  }
  virtual void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone,
                                     FAUSTFLOAT min, FAUSTFLOAT max){};
  virtual void addVerticalBargraph(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT min, FAUSTFLOAT max){};

  FAUSTFLOAT *getZone(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->zone;
    else
      return NULL;
  }
  MYFLT getMax(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->max;
    else
      return 0;
  }
  MYFLT getMin(char *label) {
    ctl *pctl = &anchor;
    pctl = pctl->nxt;
    while (pctl) {
      if (strcmp(pctl->label, label) == 0)
        break;
      pctl = pctl->nxt;
    }
    if (pctl)
      return pctl->min;
    else
      return 0;
  }
};

/**
 * Faust object handle
 *
 **/
struct faustobj {
  void *obj;
  controls *ctls;
  faustobj *nxt;
  uint64_t cnt;
};

/**
 * Faust compile opcode

 usage:
 ihandle  faustcompile Scode, Sargs[,iasync, istacksize]

 ihandle - handle to compiled code
 Scode - Faust program
 Sargs - Faust compiler args
 istacksize - compiler stack size in megabytes (default 1MB).
 iasync - async operation (1 = on, 0 = off) default to 1
**/
struct faustcompile {
  OPDS h;
  MYFLT *hptr;
  STRINGDAT *code;
  STRINGDAT *args;
  MYFLT *async;
  MYFLT *stacksize;
  MYFLT *extra;
  llvm_dsp_factory *factory;
  uintptr_t thread;
  pthread_mutex_t *lock;
};

char **parse_cmd(CSOUND *csound, char *str, int32_t *argc) {
  char **argv;
  int32_t i = 0, n = 0, end = strlen(str);
  while (str[i] == ' ')
    i++;
  if (str[i] != '\0')
    *argc = 1;
  while (str[i] != '\0') {
    if (str[i] == ' ') {
      while (str[++i] == ' ')
        ;
      if (str[i] == '\0')
        break;
      (*argc)++;
    }
    i++;
  }
  argv = (char **)csound->Calloc(csound, sizeof(char *) * (*argc));
  i = 0;
  while (str[i] == ' ')
    i++;
  for (n = 0; n < *argc && i < end; n++) {
    argv[n] = &(str[i]);
    while (str[++i] != ' ' && i < end)
      ;
    if (i >= end)
      break;
    str[i] = '\0';
    while (str[++i] == ' ' && i < end)
      ;
  }
  return argv;
}

int32_t delete_faustcompile(CSOUND *csound, void *p) {

  faustcompile *pp = ((faustcompile *)p);
  faustobj *fobj, *prv, **pfobj;
#ifdef HAVE_PTHREAD
  pthread_join((pthread_t) pp->thread, NULL);
#else
  csound->JoinThread((void *) pp->thread);
#endif
  pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::factory");
  if(pfobj != NULL) {
    fobj = *pfobj;
    prv = fobj;
    while (fobj != NULL) {
      if (fobj->obj == (void *)pp->factory) {
        prv->nxt = fobj->nxt;
        break;
      }
      prv = fobj;
      fobj = fobj->nxt;
    }
    if (fobj != NULL) {
      if (*pfobj == fobj)
        *pfobj = fobj->nxt;
      deleteDSPFactory(pp->factory);
      csound->Free(csound, fobj);
    }
  }
  return OK;
}

struct hdata {
  faustcompile *p;
  CSOUND *csound;
};

void *init_faustcompile_thread(void *pp) {

  faustcompile *p = ((hdata *)pp)->p;
  faustobj **pffactory, *ffactory;
  CSOUND *csound = ((hdata *)pp)->csound;
  llvm_dsp_factory *factory;
  int32_t argc = 0;
  std::string err_msg;
  char *cmd = (char *) csound->Calloc(csound, p->args->size + 9);
  char *ccode = csound->Strdup(csound, p->code->data);
  char *extra;
  MYFLT test = *p->extra;
  int32_t ret;
  strcpy(cmd, p->args->data);
#ifdef USE_DOUBLE
  strcat(cmd, " -double");
#endif
  const char **argv = (const char **) parse_cmd(csound, cmd, &argc);
  const char *varname = "::factory";

  if(test)
    extra = ((STRINGDAT *) p->extra)->data;
  else
    extra = (char *) "";

  // Need to protect this
  csound->LockMutex(p->lock);
  // csound->Message(csound, "lock %p\n", p->lock);
  factory = createDSPFactoryFromString("faustop", (const char *) ccode,
                                       argc, argv, extra, err_msg, 3);
  // csound->Message(csound, "unlock %p\n", p->lock);
  csound->UnlockMutex(p->lock);

  if (factory == NULL) {
    csound->Message(csound, Str("\nFaust compilation problem:\nline %s\n"),
                    err_msg.c_str());
    *(p->hptr) = FL(-2.0); // error code.
    csound->Free(csound, argv);
    csound->Free(csound, cmd);
    csound->Free(csound, ccode);
    csound->Free(csound, pp);
    ret = -1;
#ifdef HAVE_PTHREAD
    pthread_exit(&ret);
#else
    return NULL;
#endif
  }

  pffactory = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pffactory == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pffactory = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    ffactory = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    ffactory->obj = factory;
    ffactory->nxt = NULL;
    ffactory->cnt = 0ul;
    *pffactory = ffactory;
  } else {
    ffactory = *pffactory;
    while (ffactory->nxt) {
      ffactory = ffactory->nxt;
    }
    ffactory->nxt = (faustobj *) csound->Calloc(csound, sizeof(faustobj));
    ffactory->nxt->cnt = ffactory->cnt+1ul;
    ffactory = ffactory->nxt;
    ffactory->obj = factory;
  }
  p->factory = factory;
  if(p->hptr)
    *p->hptr = FL(ffactory->cnt);
  csound->Free(csound, argv);
  csound->Free(csound, cmd);
  csound->Free(csound, ccode);
  csound->Free(csound, pp);

  //csound->Message(csound, "Successfully compiled faust code\n");
  return NULL;
}

#define MBYTE 1048576
int32_t init_faustcompile(CSOUND *csound, faustcompile *p) {
  uintptr_t thread;
  hdata *data = (hdata *)csound->Malloc(csound, sizeof(hdata));
  data->csound = csound;
  data->p = p;
  *p->hptr = -1.0;

  p->lock =
    (pthread_mutex_t *)csound->QueryGlobalVariable(csound, "::faustlock::");
  if (p->lock == NULL) {
    csound->CreateGlobalVariable(csound,
                                 "::faustlock::", sizeof(pthread_mutex_t));
    p->lock =
      (pthread_mutex_t *)csound->QueryGlobalVariable(csound, "::faustlock::");
    pthread_mutex_init(p->lock, NULL);
    // csound->Message(csound, "lock created %p\n", p->lock);
  }


#ifdef HAVE_PTHREAD
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, *p->stacksize * MBYTE);
  pthread_create((pthread_t *) &thread, &attr, init_faustcompile_thread, data);
#else
  // FIXME: for systems with no pthreads (e.g. Windows - MSVC)
  // a means of setting the stack size will need to be found
  thread = (uintptr_t)
    csound->CreateThread((uintptr_t (*)(void *))init_faustcompile_thread, data);
#endif

  p->thread = thread;
  if(!(int)*p->async) {
#ifdef HAVE_PTHREAD
    int32_t *ret;
    pthread_join((pthread_t) thread, (void **)&ret);
    if (ret == NULL)
      return OK;
    else
      return NOTOK;
#else
    csound->JoinThread((void *) thread);
    return OK;
#endif
  }
  else csound->RegisterResetCallback(csound, p, delete_faustcompile);
  return OK;
}






struct faustdsp {
  OPDS h;
  MYFLT *ohptr;
  MYFLT *code;           /* faust compiled code handle */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
};

/* deinit function
   delete faust dsp objects
*/
int32_t delete_faustdsp(CSOUND *csound, void *p) {
  faustdsp *pp = (faustdsp *)p;
  faustobj *fobj, *prv, **pfobj;
  pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  fobj = *pfobj;
  prv = fobj;
  while (fobj != NULL) {
    if (fobj->obj == (void *)pp->engine) {
      prv->nxt = fobj->nxt;
      break;
    }
    prv = fobj;
    fobj = fobj->nxt;
  }
  if (fobj != NULL) {
    if (*pfobj == fobj)
      *pfobj = fobj->nxt;
    csound->Free(csound, fobj);
    delete pp->engine;
  } else
    csound->Warning(csound, Str("could not find DSP %p for deletion"),
                    pp->engine);
  if (pp->factory)
    deleteDSPFactory(pp->factory);

  return OK;
}

int32_t init_faustdsp(CSOUND *csound, faustdsp *p) {
  int32_t factory;
  faustobj *fobj, **fobjp, **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";
  int timout = 0;
  while (*((MYFLT *)p->code) == -1.0) {
    csound->Sleep(1);
    timout++;
    if(timout > 1000) {
      return csound->InitError(
                               csound, "%s", Str("Faust code was not ready. Try compiling it \n"
                                                 "in a separate instrument prior to running it here\n"));
    }
  }

  factory = (int32_t)*((MYFLT *)p->code);

  if (factory == -2)
    return csound->InitError(
                             csound, "%s", Str("Faust code did not compile properly.\n"
                                               "Check above messages for Faust compiler errors\n"));

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::factory");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no factory available\n"));
  fobj = *fobjp;
  while ((int32_t)fobj->cnt != factory) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("factory not found %d\n"),
                               (int32_t)factory);
  }

  dsp = ((llvm_dsp_factory *)fobj->obj)->createDSPInstance();
  if (dsp == NULL)
    return csound->InitError(csound, "%s", Str("Faust instantiation problem\n"));

  dsp->buildUserInterface(ctls);
  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    if (fdsp != NULL) {
      while (fdsp->nxt) {
        fdsp = fdsp->nxt;
      }
      fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->nxt->cnt = fdsp->cnt + 1;
      fdsp = fdsp->nxt;
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
    } else {
      fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
      fdsp->nxt = NULL;
      fdsp->cnt = 0;
      *pfdsp = fdsp;
    }
  }

  p->factory = NULL; // this opcode does not own the factory
  p->engine = (llvm_dsp *)fdsp->obj;
  p->engine->init(csound->GetSr(csound));
  csound->RegisterDeinitCallback(csound, p, delete_faustdsp);
  *p->ohptr = (MYFLT)fdsp->cnt;
  return OK;

}

struct faustplay {
  OPDS h;
  MYFLT *outs[MAXARG];       /* outputs */
  MYFLT *inst;
  MYFLT *ins[VARGMAX];       /* inputs */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
  AUXCH memin;
  AUXCH memout;
#ifndef USE_DOUBLE
  AUXCH buffin;
  AUXCH buffout;
#endif
};


int32_t init_faustplay(CSOUND *csound, faustplay *p) {
  faustobj *fobj, **fobjp;
  OPARMS parms;
  int32_t instance = (int32_t)*p->inst;

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));
  fobj = *fobjp;

  while ((int32_t)fobj->cnt != instance) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("dsp instance not found %d\n"),
                               (int32_t)*p->inst);
  }

  p->engine = (llvm_dsp *)fobj->obj;
  p->engine->init(csound->GetSr(csound));

  if (p->engine->getNumInputs() != p->INCOUNT - 1) {
    delete p->engine;
    return csound->InitError(csound, "%s", Str("wrong number of input args\n"));
  }
  if (p->engine->getNumOutputs() != p->OUTCOUNT) {
    delete p->engine;
    return csound->InitError(csound, "%s", Str("wrong number of output args\n"));
  }

  /* memory for sampAccurate offsets */
  csound->GetOParms(csound, &parms);
  if (parms.sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(MYFLT *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(MYFLT *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }

#ifndef USE_DOUBLE
  {
    size_t size;
    size = CS_KSMPS * p->engine->getNumInputs() * sizeof(double);
    if (p->buffin.auxp == NULL || p->buffin.size < size)
      csound->AuxAlloc(csound, size, &p->buffin);
    size = CS_KSMPS * p->engine->getNumOutputs() * sizeof(double);
    if (p->buffout.auxp == NULL || p->buffout.size < size)
      csound->AuxAlloc(csound, size, &p->buffout);
  }
#endif
  return OK;
}

int32_t perf_faustplay(CSOUND *csound, faustplay *p) {
  int32_t nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  MYFLT **in_tmp = (MYFLT **)p->memin.auxp;
  MYFLT **out_tmp = (MYFLT **)p->memout.auxp;
  AVOIDDENORMALS;

  if (UNLIKELY(early)) {
    for (i = 0; i < p->OUTCOUNT - 1; i++)
      memset(p->outs[i], '\0', nsmps * sizeof(MYFLT));
    nsmps -= early;
  }
  if (UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTCOUNT; i++) {
      memset(p->outs[i], '\0', nsmps * sizeof(MYFLT));
      out_tmp[i] = p->outs[i];
      p->outs[i] = &(p->outs[i][offset]);
    }
    for (i = 0; i < p->INCOUNT - 1; i++) {
      in_tmp[i] = p->ins[i];
      p->ins[i] = &(p->ins[i][offset]);
    }
    nsmps -= offset;
  }

#ifdef USE_DOUBLE
  p->engine->compute(nsmps, p->ins, p->outs);
#else
  {
    int n;
    double *buffin, **buffinp = (double **) p->buffin.auxp;
    double *buffout, **buffoutp = (double **) p->buffout.auxp;
    for (i = 0; i < p->INCOUNT - 1; i++) {
      buffin = ((double *)p->buffin.auxp) + CS_KSMPS*i;
      if(UNLIKELY(offset))
        memset(buffin, '\0', offset * sizeof(MYFLT));
      for(n = offset; n < nsmps; n++)
        buffin[n] = p->ins[i][n];
    }
    p->engine->compute(nsmps,buffinp, buffoutp);
    for (i = 0; i < p->OUTCOUNT - 1; i++) {
      buffout = ((double *)p->buffout.auxp) + CS_KSMPS*i;
      for(n = offset; n < nsmps; n++)
        p->ins[i][n] = buffout[n];
    }
  }
#endif

  if (UNLIKELY(offset)) {
    /* restore pos  */
    for (i = 0; i < p->OUTCOUNT; i++)
      p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INCOUNT - 1; i++)
      p->ins[i] = in_tmp[i];
  }
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
  MYFLT *outs[MAXARG];       /* outputs */
  STRINGDAT *code;           /* faust code as string */
  MYFLT *ins[VARGMAX];       /* inputs */
  llvm_dsp *engine;          /* faust DSP */
  llvm_dsp_factory *factory; /* DSP factory */
  controls *ctls;
  AUXCH memin;
  AUXCH memout;
#ifndef USE_DOUBLE
  AUXCH buffin;
  AUXCH buffout;
#endif
};

struct hdata2 {
  faustgen *p;
  CSOUND *csound;
};


/* deinit function
   delete faust objects
*/
int32_t delete_faustgen(CSOUND *csound, void *p) {
  faustgen *pp = (faustgen *)p;
  faustobj *fobj, *prv, **pfobj;
  if((pfobj = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp"))
     != NULL) {
    fobj = *pfobj;
    prv = fobj;
    while (fobj != NULL) {
      if (fobj->obj == (void *)pp->engine) {
        prv->nxt = fobj->nxt;
        break;
      }
      prv = fobj;
      fobj = fobj->nxt;
    }
    if (fobj != NULL) {
      if (*pfobj == fobj)
        *pfobj = fobj->nxt;
      csound->Free(csound, fobj);
      delete pp->ctls;
      delete pp->engine;
    } /*else
        csound->Warning(csound, Str("could not find DSP %p for deletion"),
        pp->engine);*/
  }
  if (pp->factory)
    deleteDSPFactory(pp->factory);

  return OK;
}


int32_t init_faustaudio(CSOUND *csound, faustgen *p) {
  int32_t factory;
  OPARMS parms;
  faustobj *fobj, **fobjp, **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";
  int timout = 0;
  while (*((MYFLT *)p->code) == -1.0) {
    csound->Sleep(1);
    timout++;
    if(timout > 1000) {
      return csound->InitError(
                               csound, "%s", Str("Faust code was not ready. Try compiling it \n"
                                                 "in a separate instrument prior to running it here\n"));
    }
  }


  factory = (int32_t)*((MYFLT *)p->code);

  if (factory == -2)
    return csound->InitError(
                             csound, "%s", Str("Faust code did not compile properly.\n"
                                               "Check above messages for Faust compiler errors\n"));

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::factory");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no factory available\n"));
  fobj = *fobjp;
  while ((int32_t)fobj->cnt != factory) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("factory not found %d\n"),
                               (int32_t)factory);
  }

  dsp = ((llvm_dsp_factory *)fobj->obj)->createDSPInstance();
  if (dsp == NULL)
    return csound->InitError(csound, "%s", Str("Faust instantiation problem\n"));

  dsp->buildUserInterface(ctls);
  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    if (fdsp != NULL) {
      while (fdsp->nxt) {
        fdsp = fdsp->nxt;
      }
      fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->nxt->cnt = fdsp->cnt + 1;
      fdsp = fdsp->nxt;
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
    } else {
      fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
      fdsp->obj = dsp;
      fdsp->ctls = ctls;
      fdsp->nxt = NULL;
      fdsp->cnt = 0;
      *pfdsp = fdsp;
    }
  }

  p->factory = NULL; // this opcode does not own the factory
  p->engine = (llvm_dsp *)fdsp->obj;
  p->engine->init(csound->GetSr(csound));

  if (p->engine->getNumInputs() != p->INCOUNT - 1) {
    delete p->engine;
    p->engine = NULL;
    return csound->InitError(csound, "%s", Str("wrong number of input args\n"));
  }
  if (p->engine->getNumOutputs() != p->OUTCOUNT - 1) {
    delete p->engine;
    p->engine = NULL;
    return csound->InitError(csound, "%s", Str("wrong number of output args\n"));
  }

  /* memory for sampAccurate offsets */
  csound->GetOParms(csound, &parms);
  if (parms.sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(MYFLT *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(MYFLT *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }

#ifndef USE_DOUBLE
  {
    size_t size;
    size = CS_KSMPS * p->engine->getNumInputs() * sizeof(double);
    if (p->buffin.auxp == NULL || p->buffin.size < size)
      csound->AuxAlloc(csound, size, &p->buffin);
    size = CS_KSMPS * p->engine->getNumOutputs() * sizeof(double);
    if (p->buffout.auxp == NULL || p->buffout.size < size)
      csound->AuxAlloc(csound, size, &p->buffout);
  }
#endif

  p->ctls = ctls;
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  *p->ohptr = (MYFLT)fdsp->cnt;
  return OK;
}

void *init_faustgen_thread(void *pp) {
  CSOUND *csound = ((hdata2 *)pp)->csound;
  faustgen *p = ((hdata2 *)pp)->p;
  OPARMS parms;
  std::string err_msg;
  int32_t argc = 3;
  const char *argv[argc];
  faustobj **pfdsp, *fdsp;
  llvm_dsp *dsp;
  controls *ctls = new controls();
  const char *varname = "::dsp";
  argv[0] = "-vec";
  argv[1] = "-lv";
  argv[2] = " 1";
  p->engine = NULL;

#ifdef USE_DOUBLE
  argv[3] = "-double";
  argc += 1;
#endif

  p->factory = createDSPFactoryFromString(
                                          "faustop", (const char *)p->code->data, argc, argv, "", err_msg, 3);
  if (p->factory == NULL) {
    int32_t ret = csound->InitError(csound, Str("Faust compilation problem: %s\n"),
                                    err_msg.c_str());
    csound->Free(csound, pp);
#ifdef HAVE_PTHREAD
    pthread_exit(&ret);
#else
    return NULL;
#endif
  }

  dsp = p->factory->createDSPInstance();
  if (dsp == NULL) {
    int32_t ret = csound->InitError(csound, "%s",
                                    Str("Faust instantiation problem\n"));
    csound->Free(csound, pp);
#ifdef HAVE_PTHREAD
    pthread_exit(&ret);
#else
    return NULL;
#endif
  }

  dsp->buildUserInterface(ctls);

  pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
  if (pfdsp == NULL ||  *pfdsp == NULL) {
    csound->CreateGlobalVariable(csound, varname, sizeof(faustobj *));
    pfdsp = (faustobj **)csound->QueryGlobalVariable(csound, varname);
    fdsp = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
    fdsp->nxt = NULL;
    fdsp->cnt = 0;
    *pfdsp = fdsp;
  } else {
    fdsp = *pfdsp;
    while (fdsp->nxt) {
      fdsp = fdsp->nxt;
    }
    fdsp->nxt = (faustobj *)csound->Calloc(csound, sizeof(faustobj));
    fdsp->nxt->cnt = fdsp->cnt++;
    fdsp = fdsp->nxt;
    fdsp->obj = dsp;
    fdsp->ctls = ctls;
  }

  p->engine = dsp;
  dsp->buildUserInterface(ctls);
  dsp->init(csound->GetSr(csound));
  if (p->engine->getNumInputs() != p->INCOUNT - 1) {
    int32_t ret;
    ret = csound->InitError(csound, "%s", Str("wrong number of input args\n"));
    delete p->engine;
    deleteDSPFactory(p->factory);
    p->factory = NULL;
    p->engine = NULL;
    csound->Free(csound, pp);
    pthread_exit(&ret);
  }
  if (p->engine->getNumOutputs() != p->OUTCOUNT - 1) {
    int32_t ret;
    ret = csound->InitError(csound,
                            Str("wrong number of output args: need %d had %d\n"),
                            p->engine->getNumOutputs(),
                            p->OUTCOUNT - 1
                            );
    delete p->engine;
    deleteDSPFactory(p->factory);
    csound->Free(csound, pp);
    p->engine = NULL;
    p->factory = NULL;
    pthread_exit(&ret);
  }

  /* memory for sampAccurate offsets */
  csound->GetOParms(csound, &parms);
  if (parms.sampleAccurate) {
    size_t size;
    size = p->engine->getNumInputs() * sizeof(MYFLT *);
    if (p->memin.auxp == NULL || p->memin.size < size)
      csound->AuxAlloc(csound, size, &p->memin);
    size = p->engine->getNumOutputs() * sizeof(MYFLT *);
    if (p->memout.auxp == NULL || p->memout.size < size)
      csound->AuxAlloc(csound, size, &p->memout);
  }
  p->ctls = ctls;
  *p->ohptr = (MYFLT)fdsp->cnt;

  csound->Free(csound, pp);
  return NULL;
}

int32_t init_faustgen(CSOUND *csound, faustgen *p) {
  uintptr_t thread;
  pthread_attr_t attr;
  int32_t *ret;
  hdata2 *data = (hdata2 *)csound->Malloc(csound, sizeof(hdata2));
  data->csound = csound;
  data->p = p;
#ifdef HAVE_PTHREAD
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, MBYTE);
  pthread_create((pthread_t *)&thread, &attr, init_faustgen_thread, data);
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  pthread_join((pthread_t)thread, (void **)&ret);

  if (ret == NULL)
    return OK;
  else
    return NOTOK;
#else
  // FIXME: for systems with no pthreads (e.g. Windows - MSVC)
  // a means of setting the stack size will need to be found
  thread = (uintptr_t)
    csound->CreateThread((uintptr_t (*)(void *))init_faustcompile_thread, data);
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  csound->JoinThread((void *)thread);
  csound->RegisterDeinitCallback(csound, p, delete_faustgen);
  return OK;
#endif


}

int32_t perf_faust(CSOUND *csound, faustgen *p) {
  int32_t nsmps = CS_KSMPS, i;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  MYFLT **in_tmp = (MYFLT **)p->memin.auxp;
  MYFLT **out_tmp = (MYFLT **)p->memout.auxp;
  AVOIDDENORMALS;

  if (UNLIKELY(early)) {
    for (i = 0; i < p->OUTCOUNT - 1; i++)
      memset(p->outs[i], '\0', nsmps * sizeof(MYFLT));
    nsmps -= early;
  }
  if (UNLIKELY(offset)) {
    /* offset pointers, save current pos */
    for (i = 0; i < p->OUTCOUNT - 1; i++) {
      memset(p->outs[i], '\0', nsmps * sizeof(MYFLT));
      out_tmp[i] = p->outs[i];
      p->outs[i] = &(p->outs[i][offset]);
    }
    for (i = 0; i < p->INCOUNT - 1; i++) {
      in_tmp[i] = p->ins[i];
      p->ins[i] = &(p->ins[i][offset]);
    }
    nsmps -= offset;
  }

#ifdef USE_DOUBLE
  p->engine->compute(nsmps, p->ins, p->outs);
#else
  {
    int n;
    double *buffin, **buffinp = (double **) p->buffin.auxp;
    double *buffout, **buffoutp = (double **) p->buffout.auxp;
    for (i = 0; i < p->INCOUNT - 1; i++) {
      buffin = ((double *)p->buffin.auxp) + CS_KSMPS*i;
      if(UNLIKELY(offset))
        memset(buffin, '\0', offset * sizeof(MYFLT));
      for(n = offset; n < nsmps; n++)
        buffin[n] = p->ins[i][n];
    }
    p->engine->compute(nsmps,buffinp, buffoutp);
    for (i = 0; i < p->OUTCOUNT - 1; i++) {
      buffout = ((double *)p->buffout.auxp) + CS_KSMPS*i;
      for(n = offset; n < nsmps; n++)
        p->ins[i][n] = buffout[n];
    }
  }
#endif

  if (UNLIKELY(offset)) {
    /* restore pos  -- coud be memcpy?*/
    for (i = 0; i < p->OUTCOUNT - 1; i++)
      p->outs[i] = out_tmp[i];
    for (i = 0; i < p->INCOUNT - 1; i++)
      p->ins[i] = in_tmp[i];
  }
  return OK;
}

#define MAXPARAM 128

/**
 * faustctl opcode

 usage:
 faustctl  idsp, Slabel, kval[, Slabel1, kval1 ...]

 idsp - handle from an existing Faust DSP instance
 Slabel - name of control (in Faust program)
 kval - value to be sent to control

**/
struct faustctl {
  OPDS h;
  MYFLT *inst;
  STRINGDAT *label;
  MYFLT *val;
  MYFLT *extraparam[MAXPARAM];
  FAUSTFLOAT *zone;
  MYFLT min, max;
  MYFLT minextra[MAXPARAM/2], maxextra[MAXPARAM/2];
  FAUSTFLOAT *zonextra[MAXPARAM/2];
};

int32_t init_faustctl(CSOUND *csound, faustctl *p) {

  faustobj *fobj, **fobjp;
  int32_t instance = (int32_t)*p->inst;

  /* checks that extra parameter count is even */
  if((p->INCOUNT - 3)%2)
    return csound->InitError(csound, "unbalanced parameter count \n");

  fobjp = (faustobj **)csound->QueryGlobalVariable(csound, "::dsp");
  if (fobjp == NULL)
    return csound->InitError(csound, "%s", Str("no dsp instances available\n"));
  fobj = *fobjp;

  while ((int32_t)fobj->cnt != instance) {
    fobj = fobj->nxt;
    if (fobj == NULL)
      return csound->InitError(csound, Str("dsp instance not found %d\n"),
                               (int32_t)*p->inst);
  }

  p->zone = fobj->ctls->getZone(p->label->data);
  if (p->zone == NULL)
    return csound->InitError(csound, Str("dsp control %s not found\n"),
                             p->label->data);
  p->max = fobj->ctls->getMax(p->label->data);
  p->min = fobj->ctls->getMin(p->label->data);
  {
    MYFLT val = *p->val;
    if (p->min != p->max)
      val = val < p->min ? p->min : (val > p->max ? p->max : val);
    *p->zone = val;
  }


  /* implementation of extra optional parameters */
  for(int n = 0; n < p->INCOUNT - 3; n+=2) {
    char *name = ((STRINGDAT *)p->extraparam[n])->data;
    p->zonextra[n/2] = fobj->ctls->getZone(name);
    if (p->zonextra[n/2] == NULL)
      return csound->InitError(csound, Str("dsp control %s not found\n"),
                               name);
    p->maxextra[n/2] = fobj->ctls->getMax(name);
    p->minextra[n/2] = fobj->ctls->getMin(name);
    {
      MYFLT val = *(p->extraparam[n+1]);
      MYFLT min = p->minextra[n/2];
      MYFLT max = p->maxextra[n/2];
      if (min != max)
        val = val < min ? min : (val > max ? max : val);
      *(p->zonextra[n/2]) = val;
    }
  }

  return OK;
}

int32_t perf_faustctl(CSOUND *csound, faustctl *p) {
  MYFLT val = *p->val;
  if (p->min != p->max)
    val = val < p->min ? p->min : (val > p->max ? p->max : val);
  *p->zone = val;

  /* implementation of extra optional parameters */
  for(int n = 0; n < p->INCOUNT - 3; n+=2) {
    MYFLT val = *(p->extraparam[n+1]);
    MYFLT min = p->minextra[n/2];
    MYFLT max = p->maxextra[n/2];
    if (min != max)
      val = val < min ? min : (val > max ? max : val);
    *(p->zonextra[n/2]) = val;
  }
  return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
  {(char *)"faustgen", S(faustgen), 0, 3,
   (char *)"immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"SM",
   (SUBR)init_faustgen, (SUBR)perf_faust},
  {(char *)"faustcompile", S(faustcompile), 0, 1, (char *)"i", (char *)"SSppo",
   (SUBR)init_faustcompile, NULL, NULL},
  {(char *)"faustaudio", S(faustgen), 0, 3,
   (char *)"immmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"iM",
   (SUBR)init_faustaudio, (SUBR)perf_faust},
  {(char *)"faustdsp", S(faustdsp), 0, 1,
   (char *)"i", (char *)"i",
   (SUBR)init_faustdsp},
  {(char *)"faustplay", S(faustplay), 0, 3,
   (char *)"mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm", (char *)"iM",
   (SUBR)init_faustplay, (SUBR)perf_faustplay},
  {(char *)"faustctl.i", S(faustgen), 0, 1, (char *)"", (char *)"iSiN",
   (SUBR)init_faustctl},
  {(char *)"faustctl.k ", S(faustgen), 0, 3, (char *)"", (char *)"iSkN",
   (SUBR)init_faustctl, (SUBR)perf_faustctl}};

PUBLIC int64_t csound_opcode_init(CSOUND *csound, OENTRY **ep) {
  IGN(csound);
  *ep = localops;
  return (int64_t)sizeof(localops);
}

PUBLIC int32_t csoundModuleInfo(void) {
  return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int32_t)sizeof(MYFLT));
}
