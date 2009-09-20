/*
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
 * to your rawwaves directory, which contains raw soundfiles with function table
 * data.
 *
 * All these opcodes are named "STK" + the STK classname,
 * e.g. "STKBowed" for the Bowed instrument.
 *
 * All the STK opcodes have the same signature:
 *
 * aout STKName ifrequency igain {kcontroller0, kvalue1,...,kcontroller3, kvalue3}
 *
 * They take a frequency in Hertz and a gain parameter in the range [0, 1],
 * plus up to four optional krate controller-value pairs, and return an arate
 * signal that should be more or less in the range [-1, +1].
 * See the STK class documentation to determine the controller numbers
 * used by each instrument.
 */
#include <Stk.h>
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

using namespace stk;

#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <vector>

using namespace std;

#include <OpcodeBase.hpp>

#include "csGblMtx.h"

static std::map<CSOUND *, std::vector<Instrmnt *> > &getStkInstances()
{
    static std::map<CSOUND *, std::vector<Instrmnt *> > stkInstances;
    return stkInstances;
}

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
          Stk::setSampleRate(csound->esr);
          instrument = new T();
          getStkInstances()[csound].push_back(instrument);
        }
      ksmps = csound->ksmps;
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
          // memset(aoutput, 0, ksmps*sizeof(MYFLT));
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
      if(!instrument) {
        Stk::setSampleRate(csound->esr);
        instrument = new T((StkFloat) 10.0);
        getStkInstances()[csound].push_back(instrument);
      }
      ksmps = csound->ksmps;
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
          // memset(aoutput, 0, ksmps*sizef(MYFLT));
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
        (char*)"STKBandedWG",
        sizeof(STKInstrumentAdapter<BandedWG>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BandedWG>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BandedWG>::kontrol_,
      },
      {
        (char*)"STKBeeThree",
        sizeof(STKInstrumentAdapter<BeeThree>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BeeThree>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BeeThree>::kontrol_,
      },
      {
        (char*)"STKBlowBotl",
        sizeof(STKInstrumentAdapter<BlowBotl>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<BlowBotl>::init_,
        0,
        (SUBR) STKInstrumentAdapter<BlowBotl>::kontrol_,
      },
      {
        (char*)"STKBlowHole",
        sizeof(STKInstrumentAdapter1<BlowHole>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<BlowHole>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<BlowHole>::kontrol_,
      },
      {
        (char*)"STKBowed",
        sizeof(STKInstrumentAdapter1<Bowed>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Bowed>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Bowed>::kontrol_,
      },
      {
        (char*)"STKBrass",
        sizeof(STKInstrumentAdapter1<Brass>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Brass>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Brass>::kontrol_,
      },
      {
        (char*)"STKClarinet",
        sizeof(STKInstrumentAdapter1<Clarinet>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Clarinet>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Clarinet>::kontrol_,
      },
      {
        (char*)"STKDrummer",
        sizeof(STKInstrumentAdapter<Drummer>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Drummer>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Drummer>::kontrol_,
      },
      {
        (char*)"STKFlute",
        sizeof(STKInstrumentAdapter1<Flute>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Flute>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Flute>::kontrol_,
      },
      {
        (char*)"STKFMVoices",
        sizeof(STKInstrumentAdapter<FMVoices>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<FMVoices>::init_,
        0,
        (SUBR) STKInstrumentAdapter<FMVoices>::kontrol_,
      },
      {
        (char*)"STKHevyMetl",
        sizeof(STKInstrumentAdapter<HevyMetl>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<HevyMetl>::init_,
        0,
        (SUBR) STKInstrumentAdapter<HevyMetl>::kontrol_,
      },
      {
        (char*)"STKMandolin",
        sizeof(STKInstrumentAdapter1<Mandolin>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Mandolin>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Mandolin>::kontrol_,
      },
      {
        (char*)"STKModalBar",
        sizeof(STKInstrumentAdapter<ModalBar>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<ModalBar>::init_,
        0,
        (SUBR) STKInstrumentAdapter<ModalBar>::kontrol_,
      },
      {
        (char*)"STKMoog",
        sizeof(STKInstrumentAdapter<Moog>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Moog>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Moog>::kontrol_,
      },
      {
        (char*)"STKPercFlut",
        sizeof(STKInstrumentAdapter<PercFlut>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<PercFlut>::init_,
        0,
        (SUBR) STKInstrumentAdapter<PercFlut>::kontrol_,
      },
      {
        (char*)"STKPlucked",
        sizeof(STKInstrumentAdapter1<Plucked>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Plucked>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Plucked>::kontrol_,
      },
      {
        (char*)"STKResonate",
        sizeof(STKInstrumentAdapter<Resonate>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Resonate>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Resonate>::kontrol_,
      },
      {
        (char*)"STKRhodey",
        sizeof(STKInstrumentAdapter<Rhodey>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Rhodey>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Rhodey>::kontrol_,
      },
      {
        (char*)"STKSaxofony",
        sizeof(STKInstrumentAdapter1<Saxofony>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<Saxofony>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<Saxofony>::kontrol_,
      },
      {
        (char*)"STKShakers",
        sizeof(STKInstrumentAdapter<Shakers>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Shakers>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Shakers>::kontrol_,
      },
      {
        (char*)"STKSimple",
        sizeof(STKInstrumentAdapter<Simple>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Simple>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Simple>::kontrol_,
      },
      {
        (char*)"STKSitar",
        sizeof(STKInstrumentAdapter<Sitar>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Sitar>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Sitar>::kontrol_,
      },
      {
        (char*)"STKStifKarp",
        sizeof(STKInstrumentAdapter1<StifKarp>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter1<StifKarp>::init_,
        0,
        (SUBR) STKInstrumentAdapter1<StifKarp>::kontrol_,
      },
      {
        (char*)"STKTubeBell",
        sizeof(STKInstrumentAdapter<TubeBell>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<TubeBell>::init_,
        0,
        (SUBR) STKInstrumentAdapter<TubeBell>::kontrol_,
      },
      {
        (char*)"STKVoicForm",
        sizeof(STKInstrumentAdapter<VoicForm>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<VoicForm>::init_,
        0,
        (SUBR) STKInstrumentAdapter<VoicForm>::kontrol_,
      },
      {
        (char*)"STKWhistle",
        sizeof(STKInstrumentAdapter<Whistle>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
        (SUBR) STKInstrumentAdapter<Whistle>::init_,
        0,
        (SUBR) STKInstrumentAdapter<Whistle>::kontrol_,
      },
      {
        (char*)"STKWurley",
        sizeof(STKInstrumentAdapter<Wurley>),
        5,
        (char*)"a",
        (char*)"iijjjjjjjj",
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
        csound->ErrorMsg(csound,
                         Str("Error: define environment variable RAWWAVE_PATH\n"
                             "(points to rawwaves directory) to use STK opcodes."));
        return 0;
      }
    else
      {
        csound_global_mutex_lock();
        Stk::setRawwavePath(path);
        csound_global_mutex_unlock();
        csound->Message(csound,
			Str("RAWWAVE_PATH: %s\n"), Stk::rawwavePath().c_str());
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
    if (getStkInstances().find(csound) != getStkInstances().end()) {
      for(size_t i = 0, n = getStkInstances()[csound].size(); i < n; ++i) {
        delete getStkInstances()[csound][i];
      }
      getStkInstances()[csound].clear();
      getStkInstances().erase(csound);
    }
    return 0;
  }

}

