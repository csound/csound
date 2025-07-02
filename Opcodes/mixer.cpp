/*
    mixer.cpp:

    Copyright (C) 2005 by Michael Gogins

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
    02110-1301 USA58

*/
#include <map>
#include <vector>
#include "OpcodeBase.hpp"

using namespace csound;

// Define ENABLE_MIXER_IDEBUG to enable i-rate debug messages.
//#define ENABLE_MIXER_IDEBUG

// Define ENABLE_MIXER_KDEBUG to enable -rate and a-rate debug messages.
//#define ENABLE_MIXER_KDEBUG

/**
 * The mixer busses are laid out:
 * busses[csound][bus][channel][frame].
 * std::map<CSOUND *, std::map<size_t, std::vector< std::vector<MYFLT> > > >
 * *busses = 0;
 *
 * The mixer send matrix is laid out:
 * matrix[csound][send][bus].
 * std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT> > > *matrix = 0;
 */

/**
 * Creates the buss if it does not already exist.
 */
static void createBuss(CSOUND *csound, size_t buss, int32_t ksmps) {
#ifdef ENABLE_MIXER_IDEBUG
  csound->Message(csound, "createBuss: csound %p buss %d...\n", csound, buss);
#endif
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>>
      *busses = 0;
  csound::QueryGlobalPointer(csound, "busses", busses);
  if ((*busses)[csound].find(buss) == (*busses)[csound].end()) {
    size_t channels = csound->GetNchnls(csound);
    size_t frames = ksmps;
    (*busses)[csound][buss].resize(channels);
    for (size_t channel = 0; channel < channels; channel++) {
      (*busses)[csound][buss][channel].resize(frames);
    }
#ifdef ENABLE_MIXER_IDEBUG
    csound->Message(csound, "createBuss: created buss.\n");
#endif
  } else {
#ifdef ENABLE_MIXER_IDEBUG
    csound->Message(csound, "createBuss: buss already exists.\n");
#endif
  }
}

/**
 * MixerSetLevel isend, ibuss, kgain
 *
 * Controls the gain of any signal route from a send to a bus
 */
struct MixerSetLevel : public OpcodeBase<MixerSetLevel> {
  // No outputs.
  // Inputs.
  MYFLT *isend;
  MYFLT *ibuss;
  MYFLT *kgain;
  // State.
  size_t send;
  size_t buss;
  std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>> *matrix;
  int32_t init(CSOUND *csound) {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSetLevel::init...\n");
#endif
    csound::QueryGlobalPointer(csound, "matrix", matrix);
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss, opds.insdshead->ksmps);
    (*matrix)[csound][send][buss] = *kgain;
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSetLevel::init: csound %p send %d buss %d gain %f\n",
         csound, send, buss, (*matrix)[csound][send][buss]);
#endif
    return OK;
  }
  int32_t kontrol(CSOUND *csound) {
    (*matrix)[csound][send][buss] = *kgain;
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSetLevel::kontrol: csound %p send %d buss "
                 "%d gain %f\n",
         csound, send, buss, (*matrix)[csound][send][buss]);
#endif
    return OK;
  }
};

/**
 * kgain MixerGetLevel isend, ibuss
 *
 * Returns the gain of any signal route from a send to a bus.
 */
struct MixerGetLevel : public OpcodeBase<MixerGetLevel> {
  //.
  MYFLT *kgain;
  // Inputs.
  MYFLT *isend;
  MYFLT *ibuss;
  // State.
  size_t send;
  size_t buss;
  std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>> *matrix;
  int32_t init(CSOUND *csound) {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerGetLevel::init...\n");
#endif
    csound::QueryGlobalPointer(csound, "matrix", matrix);
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss, opds.insdshead->ksmps);
    return OK;
  }
  int32_t noteoff(CSOUND *) { return OK; }
  int32_t kontrol(CSOUND *csound) {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerGetLevel::kontrol...\n");
#endif
    *kgain = (*matrix)[csound][send][buss];
    return OK;
  }
};
/**
 * MixerSend asignal, isend, ibus, ichannel
 *
 * Routes a signal from a send to a channel of a mixer bus.
 * The gain of the send is controlled by the previously set mixer level.
 */
struct MixerSend : public OpcodeBase<MixerSend> {
  // No outputs.
  // Inputs.
  MYFLT *ainput;
  MYFLT *isend;
  MYFLT *ibuss;
  MYFLT *ichannel;
  // State.
  size_t send;
  size_t buss;
  size_t channel;
  size_t frames;
  MYFLT *busspointer;
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>> *busses;
  std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>> *matrix;
  int32_t init(CSOUND *csound) {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSend::init...\n");
#endif
    csound::QueryGlobalPointer(csound, "busses", busses);
    csound::QueryGlobalPointer(csound, "matrix", matrix);
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss, opds.insdshead->ksmps);
    channel = static_cast<size_t>(*ichannel);
    frames = opds.insdshead->ksmps;
    busspointer = &(*busses)[csound][buss][channel].front();
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSend::init: instance %p send %d buss "
                 "%d channel %d frames %d busspointer %p\n",
         csound, send, buss, channel, frames, busspointer);
#endif
    return OK;
  }
  int32_t noteoff(CSOUND *) { return OK; }
  int32_t audio(CSOUND *csound) {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSend::audio...\n");
#endif
    MYFLT gain = (*matrix)[csound][send][buss];
    for (size_t i = 0; i < frames; i++) {
      busspointer[i] += (ainput[i] * gain);
    }
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSend::audio: instance %d send %d buss "
                 "%d gain %f busspointer %p\n",
         csound, send, buss, gain, busspointer);
#endif
    return OK;
  }
};

/**
 * asignal MixerReceive ibuss, ichannel
 *
 * Receives a signal from a channel of a bus.
 * Obviously, instruments receiving signals must be numbered higher
 * than instruments sending those signals.
 */
struct MixerReceive : public OpcodeBase<MixerReceive> {
  // Output.
  MYFLT *aoutput;
  // Inputs.
  MYFLT *ibuss;
  MYFLT *ichannel;
  // State.
  size_t buss;
  size_t channel;
  size_t frames;
  MYFLT *busspointer;
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>> *busses;
  int32_t init(CSOUND *csound) {
    csound::QueryGlobalPointer(csound, "busses", busses);
    buss = static_cast<size_t>(*ibuss);
    channel = static_cast<size_t>(*ichannel);
    frames = opds.insdshead->ksmps;
    createBuss(csound, buss, opds.insdshead->ksmps);
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerReceive::init...\n");
#endif
    busspointer = &(*busses)[csound][buss][channel].front();
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerReceive::init csound %p buss %d channel "
                 "%d frames %d busspointer %p\n",
         csound, buss, channel, frames, busspointer);
#endif
    return OK;
  }
  int32_t noteoff(CSOUND *) { return OK; }
  int32_t audio(CSOUND *csound) {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerReceive::audio...\n");
#else
    IGN(csound);
#endif
    for (size_t i = 0; i < frames; i++) {
      aoutput[i] = busspointer[i];
    }
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerReceive::audio aoutput %p busspointer %p\n", aoutput,
         buss);
#endif
    return OK;
  }
};

/**
 * MixerClear
 *
 * Clears all busses. Must be invoked after last MixerReceive.
 * You should probably use a highest-numbered instrument
 * with an indefinite duration that invokes only this opcode.
 */
struct MixerClear : public OpcodeBase<MixerClear> {
  // No output.
  // No input.
  // State.
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>> *busses;
  int32_t init(CSOUND *csound) {
    csound::QueryGlobalPointer(csound, "busses", busses);
    return OK;
  }
  int32_t audio(CSOUND *csound) {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerClear::audio...\n")
#endif
        for (std::map<size_t, std::vector<std::vector<MYFLT>>>::iterator busi =
                 (*busses)[csound].begin();
             busi != (*busses)[csound].end(); ++busi) {
      for (std::vector<std::vector<MYFLT>>::iterator channeli =
               busi->second.begin();
           channeli != busi->second.end(); ++channeli) {
        for (std::vector<MYFLT>::iterator framei = (*channeli).begin();
             framei != (*channeli).end(); ++framei) {
          *framei = 0;
        }
      }
    }
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerClear::audio\n")
#endif
        return OK;
  }
};

extern "C" {

static OENTRY localops[] = {
    {(char *)"MixerSetLevel", sizeof(MixerSetLevel), _CW,  (char *)"",
     (char *)"iik", (SUBR)&MixerSetLevel::init_, (SUBR)&MixerSetLevel::kontrol_,
     0},
    {(char *)"MixerSetLevel_i", sizeof(MixerSetLevel), _CW,  (char *)"",
     (char *)"iii", (SUBR)&MixerSetLevel::init_, 0, 0},
    {(char *)"MixerGetLevel", sizeof(MixerGetLevel), _CR,  (char *)"k",
     (char *)"ii", (SUBR)&MixerGetLevel::init_, (SUBR)&MixerGetLevel::kontrol_,
     0},
    {(char *)"MixerSend", sizeof(MixerSend), _CW,  (char *)"", (char *)"aiii",
     (SUBR)&MixerSend::init_, (SUBR)&MixerSend::audio_},
    {(char *)"MixerReceive", sizeof(MixerReceive), _CR,  (char *)"a",
     (char *)"ii", (SUBR)&MixerReceive::init_, (SUBR)&MixerReceive::audio_},
    {(char *)"MixerClear", sizeof(MixerClear), 0,  (char *)"", (char *)"",
     (SUBR)&MixerClear::init_, (SUBR)&MixerClear::audio_},
    {NULL, 0, 0, NULL, NULL, (SUBR)NULL, (SUBR)NULL, (SUBR)NULL}};

PUBLIC int32_t csoundModuleCreate_mixer(CSOUND *csound) {
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>>
      *busses = 0;
  busses =
      new std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>>;
  csound::CreateGlobalPointer(csound, "busses", busses);
  std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>> *matrix = 0;
  matrix = new std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>>;
  csound::CreateGlobalPointer(csound, "matrix", matrix);
  return OK;
}

/*
 * The mixer busses are laid out:
 * busses[csound][bus][channel][frame].
 * std::map<CSOUND *, std::map<size_t,
 *          std::vector< std::vector<MYFLT> > > > *busses = 0;
 * The mixer send matrix is laid out:
 * matrix[csound][send][bus].
 * std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT> > > *matrix = 0;
 */
PUBLIC int32_t csoundModuleDestroy_mixer(CSOUND *csound) {
  std::map<CSOUND *, std::map<size_t, std::vector<std::vector<MYFLT>>>>
      *busses = 0;
  csound::QueryGlobalPointer(csound, "busses", busses);
  if (busses) {
    for (std::map<size_t, std::vector<std::vector<MYFLT>>>::iterator busi =
             (*busses)[csound].begin();
         busi != (*busses)[csound].end(); ++busi) {
      for (std::vector<std::vector<MYFLT>>::iterator channeli =
               busi->second.begin();
           channeli != busi->second.end(); ++channeli) {
        channeli->resize(0);
      }
      busi->second.clear();
    }
    busses->clear();
    csound->DestroyGlobalVariable(csound, "busses");
    delete busses;
    busses = nullptr;
  }
  std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT>>> *matrix = 0;
  csound::QueryGlobalPointer(csound, "matrix", matrix);
  if (matrix) {
    // std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT> > >
    for (std::map<size_t, std::map<size_t, MYFLT>>::iterator matrixi =
             (*matrix)[csound].begin();
         matrixi != (*matrix)[csound].end(); ++matrixi) {
      matrixi->second.clear();
    }
    matrix->clear();
    csound->DestroyGlobalVariable(csound, "matrix");
    delete matrix;
    matrix = nullptr;
  }
  return OK;
}

int32_t destroyMixer(CSOUND *csound, void *p) {
  IGN(p);
  return csoundModuleDestroy_mixer(csound);
}

PUBLIC int32_t csoundModuleInit_mixer(CSOUND *csound) {
  OENTRY *ep = (OENTRY *)&(localops[0]);
  int32_t err = 0;

  while (ep->opname != NULL) {
    err |= csound->AppendOpcode(csound, ep->opname, ep->dsblksiz, ep->flags,
                                 ep->outypes, ep->intypes,
                                (int32_t (*)(CSOUND *, void *))ep->init,
                                (int32_t (*)(CSOUND *, void *))ep->perf,
                                (int32_t (*)(CSOUND *, void *))ep->deinit);
    ep++;
  }
  // need to register reset callback
  csound->RegisterResetCallback(csound, NULL, destroyMixer);
  return err;
}



#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  return csoundModuleCreate_mixer(csound);
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  return csoundModuleInit_mixer(csound);
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  return csoundModuleDestroy_mixer(csound);
}
#endif
} // END EXTERN C
