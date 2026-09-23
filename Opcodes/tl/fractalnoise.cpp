/*
    fractalnoise.cpp:

    Code generated with Faust 0.9.43

    (c) Tito Latini, 2012

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "../OpcodeBase.hpp"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define dv2_31 (FL(4.656612873077392578125e-10))

typedef struct opdata { OPDS h; } OPDATA;

inline int32_t lsr (int32_t x, int32_t n)
{
    return int32_t(((uint32_t)x) >> n);
}

/* VECTOR INTRINSICS */

// inline void *aligned_calloc(size_t nmemb, size_t size)
//{
//    return (void*)(((uint64_t)(calloc((nmemb*size)+15,
//                                           (sizeof(char))))+15) & 0xfffffff0);
//}

/* ABSTRACT USER INTERFACE */

struct Meta {
  void declare(const char *key, const char *value) {
    IGN(key);
    IGN(value);
  }
};

class UserInterface {
  bool fStopped;

public:
  UserInterface() : fStopped(false) {}
  virtual ~UserInterface() {}
  virtual void addButton(char *label, cs_float *zone) = 0;
  virtual void addToggleButton(char *label, cs_float *zone) = 0;
  virtual void addCheckButton(char *label, cs_float *zone) = 0;
  virtual void addVerticalSlider(char *label, cs_float *zone, cs_float init,
                                 cs_float min, cs_float max, cs_float step) = 0;
  virtual void addHorizontalSlider(char *label, cs_float *zone, cs_float init,
                                   cs_float min, cs_float max, cs_float step) = 0;
  virtual void addNumEntry(char *label, cs_float *zone, cs_float init, cs_float min,
                           cs_float max, cs_float step) = 0;
  virtual void openFrameBox(char *label) = 0;
  virtual void openTabBox(char *label) = 0;
  virtual void openHorizontalBox(char *label) = 0;
  virtual void openVerticalBox(char *label) = 0;
  virtual void closeBox() = 0;
  virtual void run() = 0;
  void stop() { fStopped = true; }
  bool stopped() { return fStopped; }
};

class csUI : public UserInterface {
private:
  cs_float *args[2];
  int32_t ctrlCount;

  void addZone(cs_float *zone) { args[ctrlCount++] = zone; }

public:
  csUI() : UserInterface(), ctrlCount(0) { args[0] = args[1] = NULL; };
  virtual ~csUI(){};

  virtual void addButton(char *label, cs_float *zone) {
    IGN(label);
    addZone(zone);
    ;
  }
  virtual void addToggleButton(char *label, cs_float *zone) {
    IGN(label);
    addZone(zone);
    ;
  }
  virtual void addCheckButton(char *label, cs_float *zone) {
    IGN(label);
    addZone(zone);
    ;
  }
  virtual void addVerticalSlider(char *label, cs_float *zone, cs_float init,
                                 cs_float min, cs_float max, cs_float step) {
    IGN(label);
    IGN(init);
    IGN(min);
    IGN(max);
    IGN(step);
    addZone(zone);
    ;
  }
  virtual void addHorizontalSlider(char *label, cs_float *zone, cs_float init,
                                   cs_float min, cs_float max, cs_float step) {
    IGN(label);
    IGN(init);
    IGN(min);
    IGN(max);
    IGN(step);
    addZone(zone);
    ;
  }
  virtual void addNumEntry(char *label, cs_float *zone, cs_float init, cs_float min,
                           cs_float max, cs_float step) {
    IGN(label);
    IGN(init);
    IGN(min);
    IGN(max);
    IGN(step);
    addZone(zone);
  }
  virtual void openFrameBox(char *) {}
  virtual void openTabBox(char *) {}
  virtual void openHorizontalBox(char *) {}
  virtual void openVerticalBox(char *) {}
  virtual void closeBox() {}
  virtual void show() {}
  virtual void run() {}

  void updateCtrlZones(cs_float *cs_amp, cs_float *cs_beta) {
    *args[0] = *cs_amp;
    *args[1] = *cs_beta;
  }
};

/* FAUST DSP */

/* abstract definition of a signal processor */
class dsp {
protected:
  int32_t fSamplingFreq;

public:
  dsp() { fSamplingFreq = -1; }
  virtual ~dsp() {}
  virtual int32_t getNumInputs() = 0;
  virtual int32_t getNumOutputs() = 0;
  virtual void buildUserInterface(UserInterface *userInterface) = 0;
  virtual void init(int32_t samplingRate) = 0;
  virtual void compute(CSOUND *csound, cs_float *output, void *p) = 0;
};

/* FAUST generated code */

class mydsp : public dsp {
private:
  int32_t iConst0;
  cs_float fConst1;
  cs_float fConst2;
  uint32_t iRec8[2];
  cs_float fConst3;
  cs_float fConst4;
  cs_float fConst5;
  cs_float fConst6;
  cs_float fConst7;
  cs_float fConst8;
  cs_float fRec7[3];
  cs_float fslider0;
  cs_float fConst9;
  cs_float fConst10;
  cs_float fConst11;
  cs_float fConst12;
  cs_float fConst13;
  cs_float fConst14;
  cs_float fRec6[3];
  cs_float fConst15;
  cs_float fConst16;
  cs_float fConst17;
  cs_float fConst18;
  cs_float fConst19;
  cs_float fConst20;
  cs_float fRec5[3];
  cs_float fConst21;
  cs_float fConst22;
  cs_float fConst23;
  cs_float fConst24;
  cs_float fConst25;
  cs_float fConst26;
  cs_float fRec4[3];
  cs_float fConst27;
  cs_float fConst28;
  cs_float fConst29;
  cs_float fConst30;
  cs_float fConst31;
  cs_float fConst32;
  cs_float fRec3[3];
  cs_float fConst33;
  cs_float fConst34;
  cs_float fConst35;
  cs_float fConst36;
  cs_float fConst37;
  cs_float fConst38;
  cs_float fRec2[3];
  cs_float fConst39;
  cs_float fConst40;
  cs_float fConst41;
  cs_float fConst42;
  cs_float fConst43;
  cs_float fConst44;
  cs_float fRec1[3];
  cs_float fRec0[2];
  cs_float fslider1;

public:
  static void metadata(Meta *m) {
    m->declare("name", "Fractal Noise");
    m->declare("author", "Tito Latini");
    m->declare("license", "GNU LGPL");
    m->declare("copyright", "Tito Latini");
    m->declare("version", "1.0");
    m->declare("music.lib/name", "Music Library");
    m->declare("music.lib/author", "GRAME");
    m->declare("music.lib/copyright", "GRAME");
    m->declare("music.lib/version", "1.0");
    m->declare("music.lib/license", "LGPL");
    m->declare("math.lib/name", "Math Library");
    m->declare("math.lib/author", "GRAME");
    m->declare("math.lib/copyright", "GRAME");
    m->declare("math.lib/version", "1.0");
    m->declare("math.lib/license", "LGPL");
  }

  virtual int32_t getNumInputs() { return 0; }
  virtual int32_t getNumOutputs() { return 1; }
  static void classInit(int32_t sr) { IGN(sr); }
  virtual void instanceInit(int32_t samplingFreq) {
    fSamplingFreq = samplingFreq;
    iConst0 = min(192000, max(1, fSamplingFreq));
    fConst1 = FL(67683.56194843161) / iConst0;
    fConst2 = (-EXP(-fConst1));
    for (int32_t i = 0; i < 2; i++)
      iRec8[i] = 0;
    fConst3 = FL(314.1592653589793) / iConst0;
    fConst4 = EXP(-fConst3);
    fConst5 = FL(461.1227396105972) / iConst0;
    fConst6 = EXP(-fConst5);
    fConst7 = -fConst6 - fConst4;
    fConst8 = -fConst6 * -fConst4;
    for (int32_t i = 0; i < 3; i++)
      fRec7[i] = 0;
    fslider0 = FL(1.75);
    fConst9 = FL(676.8356194843168) / iConst0;
    fConst10 = EXP(-fConst9);
    fConst11 = FL(993.4588265796098) / iConst0;
    fConst12 = EXP(-fConst11);
    fConst13 = -fConst12 - fConst10;
    fConst14 = -fConst12 * -fConst10;
    for (int32_t i = 0; i < 3; i++)
      fRec6[i] = 0;
    fConst15 = FL(1458.1981380662319) / iConst0;
    fConst16 = EXP(-fConst15);
    fConst17 = FL(2140.3421591014803) / iConst0;
    fConst18 = EXP(-fConst17);
    fConst19 = -fConst18 - fConst16;
    fConst20 = -fConst18 * -fConst16;
    for (int32_t i = 0; i < 3; i++)
      fRec5[i] = 0;
    fConst21 = FL(3141.5926535897916) / iConst0;
    fConst22 = EXP(-fConst21);
    fConst23 = FL(4611.22739610597) / iConst0;
    fConst24 = EXP(-fConst23);
    fConst25 = -fConst24 - fConst22;
    fConst26 = -fConst24 * -fConst22;
    for (int32_t i = 0; i < 3; i++)
      fRec4[i] = 0;
    fConst27 = FL(6768.356194843165) / iConst0;
    fConst28 = EXP(-fConst27);
    fConst29 = FL(9934.588265796094) / iConst0;
    fConst30 = EXP(-fConst29);
    fConst31 = -fConst30 - fConst28;
    fConst32 = -fConst30 * -fConst28;
    for (int32_t i = 0; i < 3; i++)
      fRec3[i] = 0;
    fConst33 = FL(14581.981380662311) / iConst0;
    fConst34 = EXP(-fConst33);
    fConst35 = FL(21403.421591014794) / iConst0;
    fConst36 = EXP(-fConst35);
    fConst37 = -fConst36 - fConst34;
    fConst38 = -fConst36 * -fConst34;
    for (int32_t i = 0; i < 3; i++)
      fRec2[i] = 0;
    fConst39 = FL(31415.926535897903) / iConst0;
    fConst40 = EXP(-fConst39);
    fConst41 = FL(46112.27396105968f) / iConst0;
    fConst42 = EXP(-fConst41);
    fConst43 = -fConst42 - fConst40;
    fConst44 = -fConst42 * -fConst40;
    for (int32_t i = 0; i < 3; i++)
      fRec1[i] = 0;
    for (int32_t i = 0; i < 2; i++)
      fRec0[i] = 0;
    fslider1 = FL(1.0);
  }
  virtual void init(int32_t samplingFreq) {
    classInit(samplingFreq);
    instanceInit(samplingFreq);
  }
  virtual void buildUserInterface(UserInterface *userInterface) {
    userInterface->openVerticalBox((char *)"fractalnoise");
    userInterface->addVerticalSlider((char *)"amp", &fslider1, FL(1.0), FL(0.0),
                                     FL(20.0), FL(0.01));
    userInterface->addVerticalSlider((char *)"beta", &fslider0, FL(1.75),
                                     FL(0.0), FL(10.0), FL(0.01));
    userInterface->closeBox();
  }
  virtual void compute(CSOUND *csound, cs_float *output, void *p) {
    int32_t nn = ((OPDATA *)p)->h.insdshead->ksmps;
    uint32_t offset = ((OPDATA *)p)->h.insdshead->ksmps_offset;
    uint32_t early = ((OPDATA *)p)->h.insdshead->ksmps_no_end;
    const uint32_t seed = (uint32_t)*csound->RandSeed31(csound);
    cs_float fSlow0 = POWER(FL(10.0), (FL(0.08333333333333333) * fslider0));
    cs_float fSlow1 = EXP(-(fConst3 * fSlow0));
    cs_float fSlow2 = EXP(-(fConst5 * fSlow0));
    cs_float fSlow3 = -fSlow2 * -fSlow1;
    cs_float fSlow4 = -fSlow2 - fSlow1;
    cs_float fSlow5 = EXP(-(fConst9 * fSlow0));
    cs_float fSlow6 = EXP(-(fConst11 * fSlow0));
    cs_float fSlow7 = -fSlow6 * -fSlow5;
    cs_float fSlow8 = -fSlow6 - fSlow5;
    cs_float fSlow9 = EXP(-(fConst15 * fSlow0));
    cs_float fSlow10 = EXP(-(fConst17 * fSlow0));
    cs_float fSlow11 = -fSlow10 * -fSlow9;
    cs_float fSlow12 = -fSlow10 - fSlow9;
    cs_float fSlow13 = EXP(-(fConst21 * fSlow0));
    cs_float fSlow14 = EXP(-(fConst23 * fSlow0));
    cs_float fSlow15 = -fSlow14 * -fSlow13;
    cs_float fSlow16 = (FL(0.0) - (fSlow14 + fSlow13));
    cs_float fSlow17 = EXP(-(fConst27 * fSlow0));
    cs_float fSlow18 = EXP(-(fConst29 * fSlow0));
    cs_float fSlow19 = -fSlow18 * -fSlow17;
    cs_float fSlow20 = -fSlow18 - fSlow17;
    cs_float fSlow21 = EXP(-(fConst33 * fSlow0));
    cs_float fSlow22 = EXP(-(fConst35 * fSlow0));
    cs_float fSlow23 = -fSlow22 * -fSlow21;
    cs_float fSlow24 = -fSlow22 - fSlow21;
    cs_float fSlow25 = EXP(-(fConst39 * fSlow0));
    cs_float fSlow26 = EXP(-(fConst41 * fSlow0));
    cs_float fSlow27 = -fSlow26 * -fSlow25;
    cs_float fSlow28 = -fSlow26 - fSlow25;
    cs_float fSlow29 = (-EXP(-(fConst1 * fSlow0)));
    cs_float fSlow30 = fslider1;
    cs_float *output0 = output;
    if (UNLIKELY(offset))
      memset(output0, '\0', offset * sizeof(cs_float));
    if (UNLIKELY(early)) {
      nn -= early;
      memset(&output0[nn], '\0', early * sizeof(cs_float));
    }
    for (int32_t i = offset; i < nn; i++) {
      // The generator wraps at 32 bits, then produces signed noise samples.
      iRec8[0] = seed + 1103515245U * iRec8[1];
      const int32_t noise = iRec8[0] <= INT32_MAX
                               ? (int32_t)iRec8[0]
                               : -1 - (int32_t)(UINT32_MAX - iRec8[0]);
      fRec7[0] =
          -((fConst8 * fRec7[2]) + (fConst7 * fRec7[1])) + (noise * dv2_31);
      fRec6[0] =
          (0 - (((fConst14 * fRec6[2]) + (fConst13 * fRec6[1])) -
                ((fSlow4 * fRec7[1]) + (fRec7[0] + (fSlow3 * fRec7[2])))));
      fRec5[0] =
          (0 - (((fConst20 * fRec5[2]) + (fConst19 * fRec5[1])) -
                ((fSlow8 * fRec6[1]) + (fRec6[0] + (fSlow7 * fRec6[2])))));
      fRec4[0] =
          (0 - (((fConst26 * fRec4[2]) + (fConst25 * fRec4[1])) -
                ((fSlow12 * fRec5[1]) + (fRec5[0] + (fSlow11 * fRec5[2])))));
      fRec3[0] =
          (0 - (((fConst32 * fRec3[2]) + (fConst31 * fRec3[1])) -
                ((fSlow16 * fRec4[1]) + (fRec4[0] + (fSlow15 * fRec4[2])))));
      fRec2[0] =
          (0 - (((fConst38 * fRec2[2]) + (fConst37 * fRec2[1])) -
                ((fSlow20 * fRec3[1]) + (fRec3[0] + (fSlow19 * fRec3[2])))));
      fRec1[0] =
          (0 - (((fConst44 * fRec1[2]) + (fConst43 * fRec1[1])) -
                ((fSlow24 * fRec2[1]) + (fRec2[0] + (fSlow23 * fRec2[2])))));
      fRec0[0] = (((fSlow28 * fRec1[1]) + (fRec1[0] + (fSlow27 * fRec1[2]))) -
                  (fConst2 * fRec0[1]));
      output0[i] = (cs_float)(fSlow30 * (fRec0[0] + (fSlow29 * fRec0[1])));
      // post processing
      fRec0[1] = fRec0[0];
      fRec1[2] = fRec1[1];
      fRec1[1] = fRec1[0];
      fRec2[2] = fRec2[1];
      fRec2[1] = fRec2[0];
      fRec3[2] = fRec3[1];
      fRec3[1] = fRec3[0];
      fRec4[2] = fRec4[1];
      fRec4[1] = fRec4[0];
      fRec5[2] = fRec5[1];
      fRec5[1] = fRec5[0];
      fRec6[2] = fRec6[1];
      fRec6[1] = fRec6[0];
      fRec7[2] = fRec7[1];
      fRec7[1] = fRec7[0];
      iRec8[1] = iRec8[0];
    }
  }
};

// typedef struct mydsp FaustCode;

typedef struct {
  OPDS h;
  cs_float *out, *kamp, *kbeta;
  mydsp *faust;
  csUI *cs_interface;
} FRACTALNOISE;

extern "C" {
int32_t fractalnoise_cleanup(CSOUND *csound, FRACTALNOISE *p) {
  IGN(csound);
  delete p->faust;
  delete p->cs_interface;
  p->faust = 0;
  p->cs_interface = 0;
  return OK;
}

int32_t fractalnoise_init(CSOUND *csound, FRACTALNOISE *p) {
  if (p->faust == nullptr) {
    p->faust = new mydsp;
    p->cs_interface = new csUI;
    p->faust->buildUserInterface(p->cs_interface);
  }
  p->faust->init((int32_t)CS_ESR);
  return OK;
}

int32_t fractalnoise_process(CSOUND *csound, FRACTALNOISE *p) {
  p->cs_interface->updateCtrlZones(p->kamp, p->kbeta);
  p->faust->compute(csound, p->out, p);
  return OK;
}

  static OENTRY localops[] = {{(char *)"fractalnoise", sizeof(FRACTALNOISE), 0, 
                               (char *)"a", (char *)"kk", (SUBR)fractalnoise_init,
                               (SUBR)fractalnoise_process, (SUBR) fractalnoise_cleanup },
                            {0, 0, 0, 0, 0, 0, 0, 0, 0}};


PUBLIC int32_t csoundModuleInit_fractalnoise(CSOUND *csound) {
  int32_t status = 0;
  for (OENTRY *oentry = &localops[0]; oentry->opname; oentry++) {
    status |= csound->AppendOpcode(csound, oentry->opname, oentry->dsblksiz,
                                   oentry->flags, 
                                   oentry->outypes, oentry->intypes,
                                   (int32_t (*)(CSOUND *, void *))oentry->init,
                                   (int32_t (*)(CSOUND *, void *))oentry->perf,
                                   (int32_t (*)(CSOUND *, void *))oentry->deinit);
  }
  return status;
}

  
#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleInfo(void) {
  return CSOUND_MODULE_INFO;
}

PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  IGN(csound);
  return OK;
} 
PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  return csoundModuleInit_fractalnoise(csound);
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  IGN(csound);
  return OK;
}
#endif
}
