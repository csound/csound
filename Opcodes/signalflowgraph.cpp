#include "OpcodeBase.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * These opcodes enable the declaration of a signal flow graph
 * that directly connects instruments in a Csound orchestra using
 * time-domain k-rate and a-rate signal flows from outlets to inlets.
 * Instruments must be defined in the orchestra in the order of depth-
 * first traversal of the signal flow graph. As instruments with
 * outlets are dynamically created, their outlets are automatically
 * connected and summed to their assigned inlets.
 * Usage would be something like this:

 * instr 1 ; Howling wind
 * ...
 *          outleta "output", asignal
 * endin
 *
 * instr 2 ; Thunder
 * ...
 *          outleta "output", asignal
 * endin
 *
 * instr 3 ; Spatialization
 * asignal1 inleta    1, "output"
 * asignal2 inleta    2, "output"
 * al, ar   reverbsc  asignal1, asignal2, ...
 *          outleta   "outleft", al
 *          outleta   "outright", ar
 * endin
 *
 * instr 4 ; Master output level
 * al       inleta    3, "outleft"
 * ar       inleta    3, "outright"
 * aenv     transeg   ...
 *          outs      al *aenv * igain, ar *aenv * igain
 * endin
 */

struct Outlet;
struct Inlet;

std::map<size_t, std::map<std::string, std::vector<Outlet *> > > outlets;

struct Outlet : public OpcodeBase<Outlet>
{
  // No outputs.
  // Inputs: "Sk" or "Sa", insno is implicit.
  MYFLT *iName;
  MYFLT *xInput;
  /**
   * Stores a pointer to this indexed by enclosing instrument number
   * and outlet name.
   */
  int init(CSOUND *csound)
  {
    std::string name = csound->strarg2name(csound,
                                           (char*) NULL,
                                           iName,
                                           (char *)"",
                                           (int) csound->GetInputArgSMask(this));
    outlets[size_t(h.insdshead->p1)][name].push_back(this);
    return OK;
  }
  /**
   * Just an excuse for Csound to assign xInput.
   */
  int kontrol(CSOUND *csound)
  {
    return OK;
  }
  /**
   * Just an excuse for Csound to assign xInput.
   */
  int audio(CSOUND *csound)
  {
    return OK;
  }
};

struct Inlet : public OpcodeBase<Inlet>
{
  // Outputs: "a" or "k".
  MYFLT *xOutput;
  // Inputs: "iS"
  MYFLT *iSourceInstrument;
  MYFLT *iSourceOutlet;
  // State.
  std::vector<Outlet *> *sourceOutlets;
  size_t ksmps;
  /**
   * Get a pointer to the vector of outlets feeding this inlet.
   */
  int init(CSOUND *csound)
  {
    // Get a pointer to the vector of outlets feeding this inlet.
    std::string sourceOutlet = csound->strarg2name(csound,
                                                   (char*) NULL,
                                                   iSourceOutlet,
                                                   (char *)"",
                                                   (int) csound->GetInputArgSMask(this));
    sourceOutlets = &outlets[size_t(*iSourceInstrument)][sourceOutlet];
    ksmps = csound->GetKsmps(csound);
    return OK;
  }
  /**
   * Sum krate values from active outlets feeding this inlet.
   */
  int kontrol(CSOUND *csound)
  {
    *xOutput = 0;
    for (size_t i = 0, n = sourceOutlets->size(); i < n; i++) {
      Outlet *sourceOutlet = (*sourceOutlets)[i];
      if (sourceOutlet->h.insdshead->actflg) {
        *xOutput += *sourceOutlet->xInput;
      }
    }
    return OK;
  }
  /**
   * Sum arate values from active outlets feeding this inlet.
   */
  int audio(CSOUND *csound)
  {
    for (size_t i = 0; i < ksmps; i++) {
      xOutput[i] = FL(0.0);
    }
    for (size_t i = 0, n = sourceOutlets->size(); i < n; i++) {
      Outlet *sourceOutlet = (*sourceOutlets)[i];
      if (sourceOutlet->h.insdshead->actflg) {
        for (size_t j = 0; j < ksmps; j++) {
          xOutput[j] += sourceOutlet->xInput[j];
        }
      }
    }
    return OK;
  }
};

extern "C"
{

  static OENTRY localops[] = {
    {
      (char*)"outletk",
      sizeof(Outlet),
      1,
      (char*)"",
      (char*)"Sk",
      (SUBR)&Outlet::init_,
      (SUBR)&Outlet::kontrol_,
      0
    },
    {
      (char*)"outleta",
      sizeof(Outlet),
      5,
      (char*)"",
      (char*)"Sa",
      (SUBR)&Outlet::init_,
      0,
      (SUBR)&Outlet::audio_
    },
    {
      (char*)"inletk",
      sizeof(Inlet),
      3,
      (char*)"k",
      (char*)"iS",
      (SUBR)&Inlet::init_,
      (SUBR)&Inlet::kontrol_,
      0
    },
    {
      (char*)"inleta",
      sizeof(Inlet),
      5,
      (char*)"a",
      (char*)"iS",
      (SUBR)&Inlet::init_,
      0,
      (SUBR)&Inlet::audio_
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
    outlets.clear();
    return err;
  }

}   // END EXTERN C

