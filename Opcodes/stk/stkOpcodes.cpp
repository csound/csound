//#include <Stk.h>
#include <BandedWG.h>
#include <OpcodeBase.hpp>

template<typename T>
class STKInstrumentAdapter0 : public OpcodeBase< STKInstrumentAdapter0<T> >
{
public:
  // Outputs.
  MYFLT *aoutput;
  // Inputs.
  MYFLT *ifrequency;
  MYFLT *iamplitude;
  MYFLT *kcontroller;
  MYFLT *kvalue;
  // State.
  T *stkobject;
  size_t ksmps;
  int frames;
  STKInstrumentAdapter0() : stkobject(0) {}
  int init(ENVIRON *csound)
  {
    if(!stkobject)
      {
	Stk::setSampleRate(csound->GetSr(csound));
	ksmps = csound->GetKsmps(csound);
	stkobject = new T;
      }
    frames = OpcodeBase< STKInstrumentAdapter0<T> >::h.insdshead->p3 * csound->GetSr(csound);
    stkobject->noteOn(*ifrequency, *iamplitude);
    return OK;
  }
  int kontrol(ENVIRON *csound)
  {
    if(frames <= 0)
      {
	stkobject->noteOff(*iamplitude);
      }
    else
      {
	stkobject->controlChange(*kcontroller, *kvalue);
	for(size_t i = 0; i < ksmps; i++)
	  {
	    aoutput[i] = stkobject->tick();
	  }
	frames -= ksmps;
      }
    return OK;
  }
  int deinit(ENVIRON *csound)
  {
    if(stkobject)
      {
	delete stkobject;
	stkobject = 0;
      }
    return OK;
  }
};

STKInstrumentAdapter0<BandedWG> bandedWG;

extern "C"
{
  OENTRY oentries[] = 
    {
      {"BandedWG",  sizeof(bandedWG),  5, "a",  "iikk", (SUBR) bandedWG.init_,  0, (SUBR) bandedWG.kontrol_,  (SUBR) bandedWG.deinit_ },
    };


  /**
   * Called by Csound to obtain the size of
   * the table of OENTRY structures defined in this shared library.
   */

  PUBLIC int opcode_size()
  {
    return sizeof(oentries);
  }

  /**
   * Called by Csound to obtain a pointer to
   * the table of OENTRY structures defined in this shared library.
   */

  PUBLIC OENTRY *opcode_init(ENVIRON *csound)
  {
    return oentries;
  }
};
