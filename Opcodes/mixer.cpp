#include <OpcodeBase.hpp>
#include <map>
#include <vector>

/**
 * The busses are laid out:
 * busses[csound][bus][channel][frame].
 */
static std::map<CSOUND *, std::map<size_t, std::vector< std::vector<MYFLT> > > > busses;

/**
 * The mixer matrix is laid out:
 * matrix[csound][send][bus].
 */
static std::map<CSOUND *, std::map<size_t, std::map<size_t, MYFLT> > > matrix;

/**
 * MixerSetLevel isend, ibuss, kgain
 *
 * Controls the gain of any signal route from a send to a bus.
 * Also creates the bus if it does not exist.
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
    //warn(csound, "MixerSetLevel::init...\n");
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    if(busses[csound].find(buss) == busses[csound].end())
      {
        size_t channels = csound->GetNchnls(csound);
        size_t frames = csound->GetKsmps(csound);
        busses[csound][buss].resize(channels);
        for(size_t channel = 0; channel < channels; channel++)
          {
            busses[csound][buss][channel].resize(frames);
          }
        //warn(csound, "MixerSetLevel::init: initialized instance %d buss %d channels %d frames %d\n", csound, buss, channels, frames);
      }
    matrix[csound][send][buss] = *kgain;
    //warn(csound, "MixerSetLevel::kontrol: instance %d send %d buss %d gain %f\n", csound, send, buss, gain);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    //warn(csound, "MixerSetLevel::kontrol...\n");
    matrix[csound][send][buss] = *kgain;
    //warn(csound, "MixerSetLevel::kontrol: instance %d send %d buss %d gain %f\n", csound, send, buss, gain);
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
};

/**
 * kgain MixerGetLevel isend, ibuss
 *
 * Returns the gain of any signal route from a send to a bus.
 */
struct MixerGetLevel : public OpcodeBase<MixerGetLevel>
{
  // Outputs.
  MYFLT *kgain;
  // Inputs.
  MYFLT *isend;
  MYFLT *ibuss;
  // State.
  size_t send;
  size_t buss;
  int init(CSOUND *csound)
  {
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
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
    //warn(csound, "MixerSend::init...\n");
    send = static_cast<size_t>(*isend);
    buss = static_cast<size_t>(*ibuss);
    channel = static_cast<size_t>(*ichannel);
    frames = csound->GetKsmps(csound);
    busspointer = &busses[csound][buss][channel].front();
    //warn(csound, "MixerSend::init: instance %d send %d buss %d channel %d frames %d busspointer 0x%x\n", csound, send, buss, channel, frames, busspointer);
    return OK;
  }
  int audio(CSOUND *csound)
  {
    //warn(csound, "MixerSend::audio...\n");
    MYFLT gain = matrix[csound][send][buss];
    for(size_t i = 0; i < frames; i++)
      {
        busspointer[i] += (ainput[i] * gain);
      }
    //warn(csound, "MixerSend::audio: instance %d send %d buss %d gain %f busspointer 0x%x\n", csound, send, buss, gain, busspointer);
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
    //warn(csound, "MixerReceive::init...\n");
    buss = static_cast<size_t>(*ibuss);
    channel = static_cast<size_t>(*ichannel);
    frames = csound->GetKsmps(csound);
    busspointer = &busses[csound][buss][channel].front();
    //warn(csound, "MixerReceive::init instance %d buss %d channel %d frames %d busspointer 0x%x\n", instance, buss, channel, frames, busspointer);
    return OK;
  }
  int audio(CSOUND *csound)
  {
    //warn(csound, "MixerReceive::audio...\n");
    for(size_t i = 0; i < frames; i++)
      {
        aoutput[i] = busspointer[i];
      }
    //warn(csound, "MixerReceive::audio aoutput 0x%x busspointer 0x%x\n", aoutput, busspointer);
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
    //warn(csound, "MixerClear::audio...\n")
    for(std::map<size_t, std::vector< std::vector<MYFLT> > >::iterator busi = busses[csound].begin(); busi != busses[csound].end(); ++busi)
      {
        for(std::vector< std::vector<MYFLT> >::iterator channeli = busi->second.begin(); channeli != busi->second.end(); ++channeli)
          {
            for(std::vector<MYFLT>::iterator framei = (*channeli).begin(); framei != (*channeli).end(); ++framei)
              {
                *framei = 0;
              }
          }
      }
    //warn(csound, "MixerClear::audio\n")
    return OK;
  }
};

extern "C"
{

  static OENTRY localops[] = {
    {
      "MixerSetLevel",
      sizeof(MixerSetLevel),
      3,
      "",
      "iik",
      (SUBR)&MixerSetLevel::init_,
      (SUBR)&MixerSetLevel::kontrol_,
      0
    },
    {
      "MixerGetLevel",
      sizeof(MixerGetLevel),
      3,
      "k",
      "ii",
      (SUBR)&MixerGetLevel::init_,
      (SUBR)&MixerGetLevel::kontrol_,
      0
    },
    {
      "MixerSend",
      sizeof(MixerSend),
      5,
      "",
      "aiii",
      (SUBR)&MixerSend::init_,
      0,
      (SUBR)&MixerSend::audio_
    },
    {
      "MixerReceive",
      sizeof(MixerReceive),
      5,
      "a",
      "ii",
      (SUBR)&MixerReceive::init_,
      0,
      (SUBR)&MixerReceive::audio_
    },
    {
      "MixerClear",
      sizeof(MixerClear),
      4,
      "",
      "",
      0,
      0,
      (SUBR)&MixerClear::audio_
    },
    { NULL, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
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
                                  ep->opname, ep->dsblksiz, ep->thread,
                                  ep->outypes, ep->intypes,
                                  (int (*)(CSOUND *, void*)) ep->iopadr,
                                  (int (*)(CSOUND *, void*)) ep->kopadr,
                                  (int (*)(CSOUND *, void*)) ep->aopadr);
      ep++;
    }
    return err;
  }

}; // END EXTERN C

