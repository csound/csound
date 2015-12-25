#include "OpcodeBase.hpp"
#include <map>
#include <vector>
#include "interlocks.h"

// Define ENABLE_MIXER_IDEBUG to enable i-rate debug messages.
//#define ENABLE_MIXER_IDEBUG

// Define ENABLE_MIXER_KDEBUG to enable -rate and a-rate debug messages.
//#define ENABLE_MIXER_KDEBUG

/**
 * The mixer busses are laid out:
 * busses[csound][bus][channel][frame].
 */
static std::map<CSOUND *, std::map<size_t,
                                   std::vector< std::vector<MYFLT> > > > busses;

/**
 * The mixer send matrix is laid out:
 * matrix[csound][send][bus].
 */
static std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT> > > matrix;

/**
 * Creates the buss if it does not already exist.
 */
static void createBuss(CSOUND *csound, size_t buss)
{
#ifdef ENABLE_MIXER_IDEBUG
  csound->Message(csound, "createBuss: csound %p buss %d...\n", csound, buss);
#endif
  if(busses[csound].find(buss) == busses[csound].end())
    {
      size_t channels = csound->GetNchnls(csound);
      size_t frames = csound->GetKsmps(csound);
      busses[csound][buss].resize(channels);
      for(size_t channel = 0; channel < channels; channel++)
        {
          busses[csound][buss][channel].resize(frames);
        }
#ifdef ENABLE_MIXER_IDEBUG
      csound->Message(csound, "createBuss: created buss.\n");
#endif
    }
  else
    {
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
struct MixerSetLevel : public OpcodeBase<MixerSetLevel>
{
  // No outputs.
  // Inputs.
  MYFLT *isend;
  MYFLT *ibuss;
  MYFLT *kgain;
  // State.
  size_t send;
  size_t buss;
  int init(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSetLevel::init...\n");
#endif
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss);
    matrix[csound][send][buss] = *kgain;
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSetLevel::init: csound %p send %d buss %d gain %f\n",
         csound, send, buss, matrix[csound][send][buss]);
#endif
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    matrix[csound][send][buss] = *kgain;
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSetLevel::kontrol: csound %p send %d buss "
         "%d gain %f\n", csound, send, buss, matrix[csound][send][buss]);
#endif
    return OK;
  }
};

extern "C" {
  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    if (busses.begin() != busses.end()) {
      busses.clear();
    }
    if (matrix.begin() != matrix.end()) {
      matrix.clear();
    }
    return 0;
  }
}

/**
 * kgain MixerGetLevel isend, ibuss
 *
 * Returns the gain of any signal route from a send to a bus.
 */
struct MixerGetLevel : public OpcodeBase<MixerGetLevel>
{
  //.
  MYFLT *kgain;
  // Inputs.
  MYFLT *isend;
  MYFLT *ibuss;
  // State.
  size_t send;
  size_t buss;
  int init(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerGetLevel::init...\n");
#endif
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss);
    return OK;
  }
  int noteoff(CSOUND *)
  {
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerGetLevel::kontrol...\n");
#endif
    *kgain = matrix[csound][send][buss];
    return OK;
  }
};
/**
 * MixerSend asignal, isend, ibus, ichannel
 *
 * Routes a signal from a send to a channel of a mixer bus.
 * The gain of the send is controlled by the previously set mixer level.
 */
struct MixerSend : public OpcodeBase<MixerSend>
{
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
  int init(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSend::init...\n");
#endif
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    createBuss(csound, buss);
    channel = static_cast<size_t>(*ichannel);
    frames = opds.insdshead->ksmps;
    busspointer = &busses[csound][buss][channel].front();
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerSend::init: instance %p send %d buss "
         "%d channel %d frames %d busspointer %p\n",
         csound, send, buss, channel, frames, busspointer);
#endif
    return OK;
  }
  int noteoff(CSOUND *)
  {
    return OK;
  }
  int audio(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSend::audio...\n");
#endif
    MYFLT gain = matrix[csound][send][buss];
    for(size_t i = 0; i < frames; i++)
      {
        busspointer[i] += (ainput[i] * gain);
      }
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerSend::audio: instance %d send %d buss "
         "%d gain %f busspointer %p\n", csound, send, buss, gain, busspointer);
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
struct MixerReceive : public OpcodeBase<MixerReceive>
{
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
  int init(CSOUND *csound)
  {
    buss = static_cast<size_t>(*ibuss);
    channel = static_cast<size_t>(*ichannel);
    frames = opds.insdshead->ksmps;
    createBuss(csound, buss);
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerReceive::init...\n");
#endif
    busspointer = &busses[csound][buss][channel].front();
#ifdef ENABLE_MIXER_IDEBUG
    warn(csound, "MixerReceive::init csound %p buss %d channel "
         "%d frames %d busspointer %p\n", csound, buss, channel,
         frames, busspointer);
#endif
    return OK;
  }
  int noteoff(CSOUND *)
  {
    return OK;
  }
  int audio(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerReceive::audio...\n");
#endif
    for(size_t i = 0; i < frames; i++)
      {
        aoutput[i] = busspointer[i];
      }
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerReceive::audio aoutput %p busspointer %p\n",
         aoutput, buss);
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
struct MixerClear : public OpcodeBase<MixerClear>
{
  // No output.
  // No input.
  // No state.
  int audio(CSOUND *csound)
  {
#ifdef ENABLE_MIXER_KDEBUG
    warn(csound, "MixerClear::audio...\n")
#endif
      for(std::map<size_t,
            std::vector< std::vector<MYFLT> > >::iterator
            busi = busses[csound].begin(); busi != busses[csound].end(); ++busi)
        {
          for(std::vector< std::vector<MYFLT> >::iterator
                channeli = busi->second.begin();
              channeli != busi->second.end(); ++channeli)
            {
              for(std::vector<MYFLT>::iterator
                    framei = (*channeli).begin();
                  framei != (*channeli).end(); ++framei)
                {
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

extern "C"
{

  static OENTRY localops[] = {
    {
      (char*)"MixerSetLevel",
      sizeof(MixerSetLevel),
      _CW,
      3,
      (char*)"",
      (char*)"iik",
      (SUBR)&MixerSetLevel::init_,
      (SUBR)&MixerSetLevel::kontrol_,
      0
    },
    {
      (char*)"MixerSetLevel_i",
      sizeof(MixerSetLevel),
      _CW,
      1,
      (char*)"",
      (char*)"iii",
      (SUBR)&MixerSetLevel::init_,
      0,
      0
    },
    {
      (char*)"MixerGetLevel",
      sizeof(MixerGetLevel),
      _CR,
      3,
      (char*)"k",
      (char*)"ii",
      (SUBR)&MixerGetLevel::init_,
      (SUBR)&MixerGetLevel::kontrol_,
      0
    },
    {
      (char*)"MixerSend",
      sizeof(MixerSend),
      _CR,
      5,
      (char*)"",
      (char*)"aiii",
      (SUBR)&MixerSend::init_,
      0,
      (SUBR)&MixerSend::audio_
    },
    {
      (char*)"MixerReceive",
      sizeof(MixerReceive),
      _CW,
      5,
      (char*)"a",
      (char*)"ii",
      (SUBR)&MixerReceive::init_,
      0,
      (SUBR)&MixerReceive::audio_
    },
    {
      (char*)"MixerClear",
      sizeof(MixerClear),
      0,
      4,
      (char*)"",
      (char*)"",
      0,
      0,
      (SUBR)&MixerClear::audio_
    },
    { NULL, 0, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
  };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    OENTRY  *ep = (OENTRY*) &(localops[0]);
    int     err = 0;

    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname, ep->dsblksiz, ep->flags,
                                  ep->thread, ep->outypes, ep->intypes,
                                  (int (*)(CSOUND *, void*)) ep->iopadr,
                                  (int (*)(CSOUND *, void*)) ep->kopadr,
                                  (int (*)(CSOUND *, void*)) ep->aopadr);
      ep++;
    }
    return err;
  }

}   // END EXTERN C

