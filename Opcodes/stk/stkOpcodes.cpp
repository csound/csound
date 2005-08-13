/*
 * CSOUND 5 OPCODES FOR PERRY COOK'S SYNTHESIS TOOLKIT IN C++ (STK) INSTRUMENT
 *
 * This code is copyright (C) 2005 by Michael Gogins. It is licensed under the
 * same terms as the Synthesis Tookit in C++ by Perry R. Cook and Gary P. Scavone.
 *
 * To compile these opcodes, copy the STK include, src, and rawwaves directories
 * to the csound5/Opcodes/stk directory as follows:
 *
 * csound5/Opcodes/stk/include
 * csound5/Opcodes/stk/src
 * csound5/Opcodes/stk/rawwaves
 *
 * Also, specify buildStkOpcodes=1 for SCons.
 *
 * To use these opcodes, define a RAWWAVE_PATH environment variable that points
 * to your rawwaves directory, which contains raw soundfiles with function table data.
 *
 * All these opcodes are named "STK" + the STK classname,
 * e.g. "STKBowed" for the Bowed instrument.
 *
 * All the STK opcodes have the same signature:
 *
 * aout STKName ifrequency igain {kcontroller0, kvalue1,...,kcontroller3, kvalue3}
 *
 * They take a frequency in Hertz and a gain parameter in the range [0, 1],
 * plus up to four optional krate controller-value pairs, and return an arate signal
 * that should be more or less in the range [-1, +1].
 * See the STK class documentation to determine the controller numbers
 * used by each instrument.
 */
#include <string>
#include <map>
#include <vector>
#include <cstdlib>

#include <BandedWG.h>
#include <BeeThree.h>
#include <BlowBotl.h>
#include <BlowHole.h>
#include <Bowed.h>
#include <Brass.h>
#include <Clarinet.h>
#include <Drummer.h>
#include <Flute.h>
#include <FMVoices.h>
#include <HevyMetl.h>
#include <Mandolin.h>
//#include <Mesh2D.h>
#include <ModalBar.h>
#include <Moog.h>
#include <PercFlut.h>
#include <Plucked.h>
#include <Resonate.h>
#include <Rhodey.h>
#include <Saxofony.h>
#include <Shakers.h>
#include <Simple.h>
#include <Sitar.h>
#include <StifKarp.h>
#include <TubeBell.h>
#include <VoicForm.h>
#include <Whistle.h>
#include <Wurley.h>

#include <OpcodeBase.hpp>

std::map<CSOUND *, std::vector<Instrmnt *> > stkInstances;

template<typename T>
class STKInstrumentAdapter : public OpcodeBase< STKInstrumentAdapter<T> >
{
public:
  // Outputs.
  MYFLT *aoutput;
  // Inputs.
  MYFLT *ifrequency;
  MYFLT *igain;
  MYFLT *kcontroller0;
  MYFLT *kvalue0;
  MYFLT *kcontroller1;
  MYFLT *kvalue1;
  MYFLT *kcontroller2;
  MYFLT *kvalue2;
  MYFLT *kcontroller3;
  MYFLT *kvalue3;
  // State.
  T *instrument;
  size_t ksmps;
  bool released;
  MYFLT oldkcontroller0;
  MYFLT oldkvalue0;
  MYFLT oldkcontroller1;
  MYFLT oldkvalue1;
  MYFLT oldkcontroller2;
  MYFLT oldkvalue2;
  MYFLT oldkcontroller3;
  MYFLT oldkvalue3;
  STKInstrumentAdapter() : instrument(0) {}
  int init(CSOUND *csound)
  {
    if(!instrument)
      {
        Stk::setSampleRate(csound->GetSr(csound));
        instrument = new T();
        stkInstances[csound].push_back(instrument);
      }
    ksmps = csound->GetKsmps(csound);
    instrument->noteOn(*ifrequency, *igain);
    released = false;
    oldkcontroller0 = -1.0;
    oldkvalue0 = -1.0;
    oldkcontroller1 = -1.0;
    oldkvalue1 = -1.0;
    oldkcontroller2 = -1.0;
    oldkvalue2 = -1.0;
    oldkcontroller3 = -1.0;
    oldkvalue3 = -1.0;
    return OK;
  }
  int noteoff(CSOUND *csound)
  {
    released = true;
    instrument->noteoff(0.5);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    if(!released)
      {
        if(*kcontroller0 != oldkcontroller0 || *kvalue0 != oldkvalue0)
          {
            instrument->controlChange(static_cast<int>(*kcontroller0), *kvalue0);
            oldkcontroller0 = *kcontroller0;
            oldkvalue0 = *kvalue0;
          }
        if(*kcontroller1 != oldkcontroller1 || *kvalue1 != oldkvalue1)
          {
            instrument->controlChange(static_cast<int>(*kcontroller1), *kvalue1);
            oldkcontroller1 = *kcontroller1;
            oldkvalue1 = *kvalue1;
          }
        if(*kcontroller2 != oldkcontroller2 || *kvalue2 != oldkvalue2)
          {
            instrument->controlChange(static_cast<int>(*kcontroller2), *kvalue2);
            oldkcontroller2 = *kcontroller2;
            oldkvalue2 = *kvalue2;
          }
        if(*kcontroller3 != oldkcontroller3 || *kvalue3 != oldkvalue3)
          {
            instrument->controlChange(static_cast<int>(*kcontroller3), *kvalue3);
            oldkcontroller3 = *kcontroller3;
            oldkvalue3 = *kvalue3;
          }
        for(size_t i = 0; i < ksmps; i++)
          {
            aoutput[i] = instrument->tick();
          }
      }
    else
      {
        for(size_t i = 0; i < ksmps; i++)
          {
            aoutput[i] = 0;
          }
      }
    return OK;
  }
};

template<typename T>
class STKInstrumentAdapter1 : public OpcodeBase< STKInstrumentAdapter1<T> >
{
public:
  // Outputs.
  MYFLT *aoutput;
  // Inputs.
  MYFLT *ifrequency;
  MYFLT *igain;
  MYFLT *kcontroller0;
  MYFLT *kvalue0;
  MYFLT *kcontroller1;
  MYFLT *kvalue1;
  MYFLT *kcontroller2;
  MYFLT *kvalue2;
  MYFLT *kcontroller3;
  MYFLT *kvalue3;
  // State.
  T *instrument;
  size_t ksmps;
  bool released;
  MYFLT oldkcontroller0;
  MYFLT oldkvalue0;
  MYFLT oldkcontroller1;
  MYFLT oldkvalue1;
  MYFLT oldkcontroller2;
  MYFLT oldkvalue2;
  MYFLT oldkcontroller3;
  MYFLT oldkvalue3;
  STKInstrumentAdapter1() : instrument(0) {}
  int init(CSOUND *csound)
  {
    if(!instrument)
      {
        Stk::setSampleRate(csound->GetSr(csound));
        instrument = new T((StkFloat) 10.0);
        stkInstances[csound].push_back(instrument);
      }
    ksmps = csound->GetKsmps(csound);
    instrument->noteOn(*ifrequency, *igain);
    released = false;
    oldkcontroller0 = -1.0;
    oldkvalue0 = -1.0;
    oldkcontroller1 = -1.0;
    oldkvalue1 = -1.0;
    oldkcontroller2 = -1.0;
    oldkvalue2 = -1.0;
    oldkcontroller3 = -1.0;
    oldkvalue3 = -1.0;
    return OK;
  }
  int noteoff(CSOUND *csound)
  {
    released = true;
    instrument->noteoff(0.5);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    if(!released)
      {
        if(*kcontroller0 != oldkcontroller0 || *kvalue0 != oldkvalue0)
          {
            instrument->controlChange(static_cast<int>(*kcontroller0), *kvalue0);
            oldkcontroller0 = *kcontroller0;
            oldkvalue0 = *kvalue0;
          }
        if(*kcontroller1 != oldkcontroller1 || *kvalue1 != oldkvalue1)
          {
            instrument->controlChange(static_cast<int>(*kcontroller1), *kvalue1);
            oldkcontroller1 = *kcontroller1;
            oldkvalue1 = *kvalue1;
          }
        if(*kcontroller2 != oldkcontroller2 || *kvalue2 != oldkvalue2)
          {
            instrument->controlChange(static_cast<int>(*kcontroller2), *kvalue2);
            oldkcontroller2 = *kcontroller2;
            oldkvalue2 = *kvalue2;
          }
        if(*kcontroller3 != oldkcontroller3 || *kvalue3 != oldkvalue3)
          {
            instrument->controlChange(static_cast<int>(*kcontroller3), *kvalue3);
            oldkcontroller3 = *kcontroller3;
            oldkvalue3 = *kvalue3;
          }
        for(size_t i = 0; i < ksmps; i++)
          {
            aoutput[i] = instrument->tick();
          }
      }
    else
      {
        for(size_t i = 0; i < ksmps; i++)
          {
            aoutput[i] = 0;
          }
      }
    return OK;
  }
};

extern "C"
{
  OENTRY oentries[] =
    {
      {
        "STKBandedWG",
        sizeof(STKInstrumentAdapter<BandedWG>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BandedWG>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BandedWG>::kontrol_,
      },
      {
        "STKBeeThree",
        sizeof(STKInstrumentAdapter<BeeThree>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BeeThree>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BeeThree>::kontrol_,
      },
      {
        "STKBlowBotl",
        sizeof(STKInstrumentAdapter<BlowBotl>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BlowBotl>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BlowBotl>::kontrol_,
      },
      {
        "STKBlowHole",
        sizeof(STKInstrumentAdapter1<BlowHole>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<BlowHole>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<BlowHole>::kontrol_,
      },
      {
        "STKBowed",
        sizeof(STKInstrumentAdapter1<Bowed>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Bowed>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Bowed>::kontrol_,
      },
      {
        "STKBrass",
        sizeof(STKInstrumentAdapter1<Brass>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Brass>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Brass>::kontrol_,
      },
      {
        "STKClarinet",
        sizeof(STKInstrumentAdapter1<Clarinet>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Clarinet>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Clarinet>::kontrol_,
      },
      {
        "STKDrummer",
        sizeof(STKInstrumentAdapter<Drummer>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Drummer>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Drummer>::kontrol_,
      },
      {
        "STKFlute",
        sizeof(STKInstrumentAdapter1<Flute>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Flute>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Flute>::kontrol_,
      },
      {
        "STKFMVoices",
        sizeof(STKInstrumentAdapter<FMVoices>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<FMVoices>::init_,
        0,
        (SUBR) STKInstrumentAdapter<FMVoices>::kontrol_,
      },
      {
        "STKHevyMetl",
        sizeof(STKInstrumentAdapter<HevyMetl>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<HevyMetl>::init_,
        0,
        (SUBR) STKInstrumentAdapter<HevyMetl>::kontrol_,
      },
      {
        "STKMandolin",
        sizeof(STKInstrumentAdapter1<Mandolin>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Mandolin>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Mandolin>::kontrol_,
      },
      {
        "STKModalBar",
        sizeof(STKInstrumentAdapter<ModalBar>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<ModalBar>::init_,
        0,
        (SUBR) STKInstrumentAdapter<ModalBar>::kontrol_,
      },
      {
        "STKMoog",
        sizeof(STKInstrumentAdapter<Moog>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Moog>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Moog>::kontrol_,
      },
      {
        "STKPercFlut",
        sizeof(STKInstrumentAdapter<PercFlut>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<PercFlut>::init_,
        0,
        (SUBR) STKInstrumentAdapter<PercFlut>::kontrol_,
      },
      {
        "STKPlucked",
        sizeof(STKInstrumentAdapter1<Plucked>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Plucked>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Plucked>::kontrol_,
      },
      {
        "STKResonate",
        sizeof(STKInstrumentAdapter<Resonate>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Resonate>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Resonate>::kontrol_,
      },
      {
        "STKRhodey",
        sizeof(STKInstrumentAdapter<Rhodey>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Rhodey>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Rhodey>::kontrol_,
      },
      {
        "STKSaxofony",
        sizeof(STKInstrumentAdapter1<Saxofony>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Saxofony>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Saxofony>::kontrol_,
      },
      {
        "STKShakers",
        sizeof(STKInstrumentAdapter<Shakers>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Shakers>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Shakers>::kontrol_,
      },
      {
        "STKSimple",
        sizeof(STKInstrumentAdapter<Simple>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Simple>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Simple>::kontrol_,
      },
      {
        "STKSitar",
        sizeof(STKInstrumentAdapter<Sitar>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Sitar>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Sitar>::kontrol_,
      },
      {
        "STKStifKarp",
        sizeof(STKInstrumentAdapter1<StifKarp>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<StifKarp>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<StifKarp>::kontrol_,
      },
      {
        "STKTubeBell",
        sizeof(STKInstrumentAdapter<TubeBell>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<TubeBell>::init_,
        0,
        (SUBR) STKInstrumentAdapter<TubeBell>::kontrol_,
      },
      {
        "STKVoicForm",
        sizeof(STKInstrumentAdapter<VoicForm>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<VoicForm>::init_,
        0,
        (SUBR) STKInstrumentAdapter<VoicForm>::kontrol_,
      },
      {
        "STKWhistle",
        sizeof(STKInstrumentAdapter<Whistle>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Whistle>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Whistle>::kontrol_,
      },
      {
        "STKWurley",
        sizeof(STKInstrumentAdapter<Wurley>),
        5,
        "a",
        "iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Wurley>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Wurley>::kontrol_,
      },
      {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
      }
    };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    const char *path = csound->GetEnv(csound, "RAWWAVE_PATH");
    if(!path)
      {
        csound->Message(csound, "Error: define environment variable RAWWAVE_PATH (points to rawwaves directory) to use STK opcodes.\n");
      }
    else
      {
        Stk::setRawwavePath(path);
      }
    int status = 0;
    for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++)
      {
        status |= csound->AppendOpcode(csound, oentry->opname,
                                       oentry->dsblksiz, oentry->thread,
                                       oentry->outypes, oentry->intypes,
                                       (int (*)(CSOUND*,void*)) oentry->iopadr,
                                       (int (*)(CSOUND*,void*)) oentry->kopadr,
                                       (int (*)(CSOUND*,void*)) oentry->aopadr);
      }
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    for(size_t i = 0, n = stkInstances[csound].size(); i < n; ++i)
      {
        delete stkInstances[csound][i];
      }
    stkInstances[csound].clear();
    return 0;
  }
};

