#include <OpcodeBase.hpp>
#include <map>
#include <vector>

/**
* The busses are laid out: 
* busses[bus][channel][frame].
*/
static std::map<MYFLT, std::vector< std::vector<MYFLT> > > busses;

/**
* The mixer matrix is laid out: 
* matrix[send][bus].
*/
static std::map<MYFLT, std::map<MYFLT, MYFLT> > matrix;

/**
* MixerGain isend, ibus, kgain
*
* Controls the gain of any signal routed from a send to a bus.
* Also creates the bus if it does not exist.
*/
struct MixerGain : public OpcodeBase<MixerGain>
{
    // No outputs.
    // Inputs.
    MYFLT *isend;
    MYFLT *ibus;
    MYFLT *kgain;
    // No state.
    int init(ENVIRON *csound)
    {
        if(busses.find(*ibus) == busses.end())
        {
            size_t channels = csound->GetNchnls(csound);
            size_t frames = csound->GetKsmps(csound);
            busses[*ibus].resize(channels);
            for(size_t channel = 0; channel < channels; channel++)
            {
                busses[*ibus][channel].resize(frames);
            }
        }
        return OK;
    }
    int kontrol(ENVIRON *csound)
    {
        matrix[*isend][*ibus] = *kgain;
        return OK;
    }
    int deinit(void *csound)
    {
        if(busses.begin() != busses.end())
        {
            busses.clear();
        }
        if(matrix.begin() != matrix.end())
        {
            matrix.clear();
        }
        return OK;
    }
};

/**
* MixerSend asignal, isend, ibus, ichannel
*
* Routes a signal from a send to a channel of a mixer bus.
* The gain of the send is controlled by the previously set matrix level.
*/
struct MixerSend : public OpcodeBase<MixerSend>
{
    // No outputs.
    // Inputs.
    MYFLT *ainput;
    MYFLT *isend;
    MYFLT *ibus;
    MYFLT *ichannel;
    // No state.
    int audio(ENVIRON *csound)
    {
        std::vector<MYFLT>::iterator busi = busses[*ibus][*ichannel].begin();
        std::vector<MYFLT>::iterator busend = busses[*ibus][*ichannel].end();
        MYFLT gain = matrix[*isend][*ibus];
        for(MYFLT *signal = ainput; busi != busend; ++busi)
        {
            *busi += (*signal++) * gain;
        }
        return OK;
    }
};

/**
* asignal MixerReceive ibus, ichannel
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
    MYFLT *ibus;
    MYFLT *ichannel;
    // No state.
    int audio(ENVIRON *csound)
    {
        std::vector<MYFLT>::iterator busi = busses[*ibus][*ichannel].begin();
        std::vector<MYFLT>::iterator busend = busses[*ibus][*ichannel].end();
        for(MYFLT *signal = aoutput; busi != busend; ++busi)
        {
            *signal++ = *busi;
        }
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
    int kontrol(ENVIRON *csound)
    {
        for(std::map<MYFLT, std::vector< std::vector<MYFLT> > >::iterator busi = busses.begin(); busi != busses.end(); ++busi)
        {
            for(std::vector< std::vector<MYFLT> >::iterator channeli = busi->second.begin(); channeli != busi->second.end(); ++channeli)
            {
                for(std::vector<MYFLT>::iterator framei = channeli->begin(); framei != channeli->end(); ++framei)
                {
                    *framei = FL(0.0);
                }
            }
        }
        return OK;
    }
};

extern "C" 
{

OENTRY  mixerOentries[] = { 
    {   
      "MixerGain",         
      sizeof(MixerGain),           
      3,  
      "",   
      "iik",      
      (SUBR)&MixerGain::init_,        
      (SUBR)&MixerGain::kontrol_,        
      0,                
      (SUBR)&MixerGain::deinit_, 
    },
    {   
      "MixerSend",         
      sizeof(MixerSend),           
      5,  
      "",   
      "aiii",      
      (SUBR)&MixerSend::init_,        
      0,                
      (SUBR)&MixerSend::audio_,        
      (SUBR)&MixerSend::deinit_, 
    },
    {   
      "MixerReceive",         
      sizeof(MixerReceive),           
      5,  
      "a",   
      "ii",      
      (SUBR)&MixerReceive::init_,        
      0,                
      (SUBR)&MixerReceive::audio_,        
      (SUBR)&MixerReceive::deinit_, 
    },
    {   
      "MixerClear",         
      sizeof(MixerClear),           
      2,  
      "",   
      "",      
      0,        
      (SUBR)&MixerClear::kontrol_,        
      0,        
      0, 
    },
  };
    
  /**
   * Called by Csound to obtain the size of
   * the table of OENTRY structures defined in this shared library.
   */
  PUBLIC int opcode_size()
  {
    return sizeof(OENTRY) * 4;
  }

  /**
   * Called by Csound to obtain a pointer to
   * the table of OENTRY structures defined in this shared library.
   */
  PUBLIC OENTRY *opcode_init(ENVIRON *csound)
  {
    return mixerOentries;
  }
    
}; // END EXTERN C
