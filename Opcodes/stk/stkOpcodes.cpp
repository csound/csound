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
 * to the csound6/Opcodes/stk directory as follows:
 *
 * csound6/Opcodes/stk/include
 * csound6/Opcodes/stk/src
 * csound6/Opcodes/stk/rawwaves
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

#define __BUILDING_LIBCSOUND
#include <csoundCore.h>
#include <csGblMtx.h>
#include <OpcodeBase.hpp>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
#include <vector>

using namespace std;

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
  MYFLT *kcontroller4;
  MYFLT *kvalue4;
  MYFLT *kcontroller5;
  MYFLT *kvalue5;
  MYFLT *kcontroller6;
  MYFLT *kvalue6;
  MYFLT *kcontroller7;
  MYFLT *kvalue7;
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
  MYFLT oldkcontroller4;
  MYFLT oldkvalue4;
  MYFLT oldkcontroller5;
  MYFLT oldkvalue5;
  MYFLT oldkcontroller6;
  MYFLT oldkvalue6;
  MYFLT oldkcontroller7;
  MYFLT oldkvalue7;
  STKInstrumentAdapter() : instrument(0) {}
  int init(CSOUND *csound)
  {
      if(!instrument)
        {
          Stk::setSampleRate(csound->GetSr(csound));
          instrument = new T();
          getStkInstances()[csound].push_back(instrument);
        }
      ksmps = OpcodeBase< STKInstrumentAdapter<T> >::opds.insdshead->ksmps;
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
      oldkcontroller4 = -1.0;
      oldkvalue4 = -1.0;
      oldkcontroller5 = -1.0;
      oldkvalue5 = -1.0;
      oldkcontroller6 = -1.0;
      oldkvalue6 = -1.0;
      oldkcontroller7 = -1.0;
      oldkvalue7 = -1.0;
      return OK;
  }
  int kontrol(CSOUND *csound)
  {
      uint32_t offset =
        OpcodeBase< STKInstrumentAdapter<T> >::opds.insdshead->ksmps_offset;
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
          if(*kcontroller4 != oldkcontroller4 || *kvalue4 != oldkvalue4)
            {
              instrument->controlChange(static_cast<int>(*kcontroller4), *kvalue4);
              oldkcontroller4 = *kcontroller4;
              oldkvalue4 = *kvalue4;
            }
          if(*kcontroller5 != oldkcontroller5 || *kvalue5 != oldkvalue5)
            {
              instrument->controlChange(static_cast<int>(*kcontroller5), *kvalue5);
              oldkcontroller5 = *kcontroller5;
              oldkvalue5 = *kvalue5;
            }
          if(*kcontroller6 != oldkcontroller6 || *kvalue6 != oldkvalue6)
            {
              instrument->controlChange(static_cast<int>(*kcontroller6), *kvalue6);
              oldkcontroller6 = *kcontroller6;
              oldkvalue6 = *kvalue6;
            }
          if(*kcontroller7 != oldkcontroller7 || *kvalue7 != oldkvalue7)
            {
              instrument->controlChange(static_cast<int>(*kcontroller7), *kvalue7);
              oldkcontroller7 = *kcontroller7;
              oldkvalue7 = *kvalue7;
            }
          memset(aoutput, '\0', offset*sizeof(MYFLT));
          for(size_t i = offset; i < ksmps; i++)
            {
              aoutput[i] = instrument->tick();
            }
        }
      else
        {
          //memset(aoutput, 0, ksmps*sizeof(MYFLT));
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
  MYFLT *kcontroller4;
  MYFLT *kvalue4;
  MYFLT *kcontroller5;
  MYFLT *kvalue5;
  MYFLT *kcontroller6;
  MYFLT *kvalue6;
  MYFLT *kcontroller7;
  MYFLT *kvalue7;
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
  MYFLT oldkcontroller4;
  MYFLT oldkvalue4;
  MYFLT oldkcontroller5;
  MYFLT oldkvalue5;
  MYFLT oldkcontroller6;
  MYFLT oldkvalue6;
  MYFLT oldkcontroller7;
  MYFLT oldkvalue7;
  STKInstrumentAdapter1() : instrument(0) {}
  int init(CSOUND *csound)
  {
      if(!instrument) {
        Stk::setSampleRate(csound->GetSr(csound));
        instrument = new T((StkFloat) 10.0);
        getStkInstances()[csound].push_back(instrument);
      }
      ksmps = OpcodeBase< STKInstrumentAdapter1<T> >::opds.insdshead->ksmps;
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
      oldkcontroller4 = -1.0;
      oldkvalue4 = -1.0;
      oldkcontroller5 = -1.0;
      oldkvalue5 = -1.0;
      oldkcontroller6 = -1.0;
      oldkvalue6 = -1.0;
      oldkcontroller7 = -1.0;
      oldkvalue7 = -1.0;
      return OK;
  }
  int kontrol(CSOUND *csound)
  {
      uint32_t offset =
        OpcodeBase< STKInstrumentAdapter1<T> >::opds.insdshead->ksmps_offset;
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
          if(*kcontroller4 != oldkcontroller4 || *kvalue4 != oldkvalue4)
            {
              instrument->controlChange(static_cast<int>(*kcontroller4), *kvalue4);
              oldkcontroller4 = *kcontroller4;
              oldkvalue4 = *kvalue4;
            }
          if(*kcontroller5 != oldkcontroller5 || *kvalue5 != oldkvalue5)
            {
              instrument->controlChange(static_cast<int>(*kcontroller5), *kvalue5);
              oldkcontroller5 = *kcontroller5;
              oldkvalue5 = *kvalue5;
            }
          if(*kcontroller6 != oldkcontroller6 || *kvalue6 != oldkvalue6)
            {
              instrument->controlChange(static_cast<int>(*kcontroller6), *kvalue6);
              oldkcontroller6 = *kcontroller6;
              oldkvalue6 = *kvalue6;
            }
          if(*kcontroller7 != oldkcontroller7 || *kvalue7 != oldkvalue7)
            {
              instrument->controlChange(static_cast<int>(*kcontroller7), *kvalue7);
              oldkcontroller7 = *kcontroller7;
              oldkvalue7 = *kvalue7;
            }
          memset(aoutput, '\0', offset*sizeof(MYFLT));
          for(size_t i = offset; i < ksmps; i++)
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
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<BandedWG>::init_,
        (SUBR) STKInstrumentAdapter<BandedWG>::kontrol_,
        0,
      },
      {
        (char*)"STKBeeThree",
        sizeof(STKInstrumentAdapter<BeeThree>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<BeeThree>::init_,
        (SUBR) STKInstrumentAdapter<BeeThree>::kontrol_,
        0,
      },
      {
        (char*)"STKBlowBotl",
        sizeof(STKInstrumentAdapter<BlowBotl>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<BlowBotl>::init_,
        (SUBR) STKInstrumentAdapter<BlowBotl>::kontrol_,
        0,
      },
      {
        (char*)"STKBlowHole",
        sizeof(STKInstrumentAdapter1<BlowHole>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<BlowHole>::init_,
        (SUBR) STKInstrumentAdapter1<BlowHole>::kontrol_,
        0,
      },
      {
        (char*)"STKBowed",
        sizeof(STKInstrumentAdapter1<Bowed>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Bowed>::init_,
        (SUBR) STKInstrumentAdapter1<Bowed>::kontrol_,
        0,
      },
      {
        (char*)"STKBrass",
        sizeof(STKInstrumentAdapter1<Brass>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Brass>::init_,
        (SUBR) STKInstrumentAdapter1<Brass>::kontrol_,
        0,
      },
      {
        (char*)"STKClarinet",
        sizeof(STKInstrumentAdapter1<Clarinet>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Clarinet>::init_,
        (SUBR) STKInstrumentAdapter1<Clarinet>::kontrol_,
        0,
      },
      {
        (char*)"STKDrummer",
        sizeof(STKInstrumentAdapter<Drummer>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Drummer>::init_,
        (SUBR) STKInstrumentAdapter<Drummer>::kontrol_,
        0,
      },
      {
        (char*)"STKFlute",
        sizeof(STKInstrumentAdapter1<Flute>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Flute>::init_,
        (SUBR) STKInstrumentAdapter1<Flute>::kontrol_,
        0,
      },
      {
        (char*)"STKFMVoices",
        sizeof(STKInstrumentAdapter<FMVoices>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<FMVoices>::init_,
        (SUBR) STKInstrumentAdapter<FMVoices>::kontrol_,
        0,
      },
      {
        (char*)"STKHevyMetl",
        sizeof(STKInstrumentAdapter<HevyMetl>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<HevyMetl>::init_,
        (SUBR) STKInstrumentAdapter<HevyMetl>::kontrol_,
        0,
      },
      {
        (char*)"STKMandolin",
        sizeof(STKInstrumentAdapter1<Mandolin>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Mandolin>::init_,
        (SUBR) STKInstrumentAdapter1<Mandolin>::kontrol_,
        0,
      },
      {
        (char*)"STKModalBar",
        sizeof(STKInstrumentAdapter<ModalBar>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<ModalBar>::init_,
        (SUBR) STKInstrumentAdapter<ModalBar>::kontrol_,
        0,
      },
      {
        (char*)"STKMoog",
        sizeof(STKInstrumentAdapter<Moog>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Moog>::init_,
        (SUBR) STKInstrumentAdapter<Moog>::kontrol_,
        0,
      },
      {
        (char*)"STKPercFlut",
        sizeof(STKInstrumentAdapter<PercFlut>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<PercFlut>::init_,
        (SUBR) STKInstrumentAdapter<PercFlut>::kontrol_,
        0,
      },
      {
        (char*)"STKPlucked",
        sizeof(STKInstrumentAdapter1<Plucked>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Plucked>::init_,
        (SUBR) STKInstrumentAdapter1<Plucked>::kontrol_,
        0,
      },
      {
        (char*)"STKResonate",
        sizeof(STKInstrumentAdapter<Resonate>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Resonate>::init_,
        (SUBR) STKInstrumentAdapter<Resonate>::kontrol_,
        0,
      },
      {
        (char*)"STKRhodey",
        sizeof(STKInstrumentAdapter<Rhodey>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Rhodey>::init_,
        (SUBR) STKInstrumentAdapter<Rhodey>::kontrol_,
        0,
      },
      {
        (char*)"STKSaxofony",
        sizeof(STKInstrumentAdapter1<Saxofony>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<Saxofony>::init_,
        (SUBR) STKInstrumentAdapter1<Saxofony>::kontrol_,
        0,
      },
      {
        (char*)"STKShakers",
        sizeof(STKInstrumentAdapter<Shakers>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Shakers>::init_,
        (SUBR) STKInstrumentAdapter<Shakers>::kontrol_,
        0,
      },
      {
        (char*)"STKSimple",
        sizeof(STKInstrumentAdapter<Simple>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Simple>::init_,
        (SUBR) STKInstrumentAdapter<Simple>::kontrol_,
        0,
      },
      {
        (char*)"STKSitar",
        sizeof(STKInstrumentAdapter<Sitar>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Sitar>::init_,
        (SUBR) STKInstrumentAdapter<Sitar>::kontrol_,
        0,
      },
      {
        (char*)"STKStifKarp",
        sizeof(STKInstrumentAdapter1<StifKarp>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter1<StifKarp>::init_,
        (SUBR) STKInstrumentAdapter1<StifKarp>::kontrol_,
        0,
      },
      {
        (char*)"STKTubeBell",
        sizeof(STKInstrumentAdapter<TubeBell>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<TubeBell>::init_,
        (SUBR) STKInstrumentAdapter<TubeBell>::kontrol_,
        0,
      },
      {
        (char*)"STKVoicForm",
        sizeof(STKInstrumentAdapter<VoicForm>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<VoicForm>::init_,
        (SUBR) STKInstrumentAdapter<VoicForm>::kontrol_,
        0,
      },
      {
        (char*)"STKWhistle",
        sizeof(STKInstrumentAdapter<Whistle>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Whistle>::init_,
        (SUBR) STKInstrumentAdapter<Whistle>::kontrol_,
        0,
      },
      {
        (char*)"STKWurley",
        sizeof(STKInstrumentAdapter<Wurley>),
        0,
        3,
        (char*)"a",
        (char*)"iiJJJJJJJJJJJJJJJJ",
        (SUBR) STKInstrumentAdapter<Wurley>::init_,
        (SUBR) STKInstrumentAdapter<Wurley>::kontrol_,
        0,
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
#ifdef DEFAULT_RAWWAVE_PATH
    if(!path)
        path = DEFAULT_RAWWAVE_PATH;
#endif
    if(!path)
      {
        csound->Warning(csound,
                        Str("STK opodes not available: define environment variable RAWWAVE_PATH\n"
                             "(points to rawwaves directory) to use STK opcodes."));
        return 0;
      }
    else
      {
        csound_global_mutex_lock();
        Stk::setRawwavePath(path);
        csound_global_mutex_unlock();
        csound->DebugMsg(csound,
                         Str("RAWWAVE_PATH: %s\n"), Stk::rawwavePath().c_str());
      }
    int status = 0;
    for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++)
      {
        status |= csound->AppendOpcode(csound, oentry->opname,
                                       oentry->dsblksiz, oentry->flags,
                                       oentry->thread,
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
      //getStkInstances()[csound].clear();
      getStkInstances().erase(csound);
    }
    return 0;
  }

}
