/**
 * T H E   S I G N A L   F L O W   G R A P H   O P C O D E S
 *
 * Michael Gogins
 *
 * These opcodes enable the use of signal flow graphs
 * (AKA asynchronous data flow graphs) in Csound orchestras.
 * Signals flow from the outlets of source instruments
 * and are summed in the inlets of sink instruments.
 * Signals may be k-rate, a-rate, or f-rate.
 * Any number of outlets may be connected to any number of inlets.
 * When a new instance of an instrument is instantiated during performance,
 * the declared connections also are automatically instantiated.
 *
 * Signal flow graphs simplify the construction of complex mixers,
 * signal processing chains, and the like. They also simplify the re-use
 * of "plug and play" instrument definitions and even entire sub-orchestras,
 * which can simply be #included and then "plugged in" to existing orchestras.
 *
 * Note that inlets and outlets are defined in instruments without reference
 * to how they are connected. Connections are defined in the orchestra header.
 * It is this separation that enables plug-in instruments.
 *
 * Inlets must be named. Instruments may be named or numbered, but in
 * either case each source instrument must be defined
 * in the orchestra before any of its sinks. Naming instruments makes
 * it easier to connect outlets and inlets in any higher-level orchestra
 * to inlets and outlets in any lower-level #included orchestra.
 *
 * O P C O D E S
 *
 * outleta Sname, asignal
 * outletk Sname, ksignal
 * outletf Sname, fsignal
 * outletv Sname, xsignal[]
 *
 * Outlets send a, k, or f-rate signals out from an instrument.
 * A- and k-rate signals may be arrays.
 *
 * The name of the outlet is implicitly qualified by the instrument name
 * or number,`so it is valid to use the same outlet name in more than one
 * instrument (but not to use the same outlet name twice in one instrument).
 *
 * asignal   inleta Sname
 * ksignal   inletk Sname
 * fsignal   inletf Sname
 * xsignal[] inletv SName
 *
 * Inlets receive a, k, or f-rate signals from outlets in
 * other instruments. A- and k-rate signals may be arrays.
 *
 * Outlets are connected to inlets of the same type using the connect
 * opcode. If arrays are used, the inlets and outlets must be a-rate
 * and the same shape.
 *
 * The name of the inlet is implicitly qualified by the instrument name,
 * or number, so it is valid to use the same inlet name in more than one
 * instrument (but not to use the same inlet name twice in one instrument).
 *
 * connect Tsource1, Soutlet1, Tsink1, Sinlet1
 *
 * The connect opcode, valid only in orchestra headers, sends the signals
 * from the indicated outlet in all instances of the indicated source
 * instrument to the indicated inlet in all instances of the indicated sink
 * instrument. Each inlet instance receives the sum of the signals in all
 * outlet instances. Thus multiple instances of an outlet may fan in to one
 * instance of an inlet, or one instance of an outlet may fan out to
 * multiple instances of an inlet.
 *
 * alwayson Tinstrument [p4, ..., pn]
 *
 * Activates the indicated instrument in the orchestra header,
 * without need for an i statement. Instruments must be
 * activated in the same order as they are defined.
 *
 * The alwayson opcode is designed to simplify
 * the definition of re-usable orchestras with
 * signal processing or effects chains and networks.
 *
 * When the instrument is activated, p1 is the insno, p2 is 0, and p3 is -1.
 * Pfields from p4 on may optionally be sent to the instrument.
 *
 * ifno ftgenonce ip1, ip2dummy, isize, igen, iarga, iargb [, ...]
 *
 * Enables the creation of function tables entirely inside
 * instrument definitions, without any duplication of data.
 *
 * The ftgenonce opcode is designed to simplify writing instrument definitions
 * that can be re-used in different orchestras simply by #including them
 * and plugging them into some output instrument. There is no need to define
 * function tables either in the score, or in the orchestra header.
 *
 * The ftgenonce opcode is similar to ftgentmp, and has identical arguments.
 * However, function tables are neither duplicated nor deleted. Instead,
 * all of the arguments to the opcode are concatenated to form the key to a
 * lookup table that points to the function table number. Thus, every request
 * to ftgenonce with the same arguments receives the same instance of the
 * function table data. Every change in the value of any ftgenonce argument
 * causes the creation of a new function table.
 *
 * TODO: The inletv opcode always crashes. Not sure why. Perhaps the
 * array operations are not ready for prime time at a-rate.
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "OpcodeBase.hpp"
#include <pstream.h>
#include "text.h"

#define SIGNALFLOWGRAPH_DEBUG 0

namespace csound {

struct SignalFlowGraph;
struct Outleta;
struct Outletk;
struct Outletf;
struct Outletkid;
struct Outletv;
struct Inleta;
struct Inletk;
struct Inletf;
struct Inletkid;
struct Inletv;
struct Connect;
struct AlwaysOn;
struct FtGenOnce;

static void* cs_sfg_ftables = 0;
static void* cs_sfg_ports = 0;

std::ostream &operator << (std::ostream &stream, const EVTBLK &a)
{
  stream << a.opcod;
  for (int i = 0; i < a.pcnt; i++) {
    stream << " " << a.p[i];
  }
  return stream;
}

/**
 * A wrapper to get proper C++ value
 * semantics for a map key.
 */
struct EventBlock {
  EVTBLK evtblk;
  EventBlock() {
    std::memset(&evtblk, 0, sizeof(EVTBLK));
  }
  EventBlock(const EVTBLK &other) {
    std::memcpy(&evtblk, &other, sizeof(EVTBLK));
  }
  EventBlock(const EventBlock &other) {
    std::memcpy(&evtblk, &other.evtblk, sizeof(EVTBLK));
  }
  virtual ~EventBlock() {
  }
  EventBlock &operator = (const EVTBLK &other) {
    std::memcpy(&evtblk, &other, sizeof(EVTBLK));
    return *this;
  }
  EventBlock &operator = (const EventBlock &other) {
    std::memcpy(&evtblk, &other.evtblk, sizeof(EVTBLK));
    return *this;
  }
};

/*
The logic here is a bit tricky. If p5 is NaN then it indicates a string.
The bytewise comparison will be equality for that pfield even if the
strings are different. So if two EVTBLKs seem equal but contain strings,
the strings also have to be compared. There is at most one string.
*/
bool operator < (const EventBlock &a, const EventBlock &b) {
    int n = std::max(a.evtblk.pcnt, b.evtblk.pcnt);
    bool result = true;
    if (std::memcmp(&a.evtblk.p[0], &b.evtblk.p[0], sizeof(MYFLT) * n) < 0) {
        result = true;
    } else {
        result = false;
    }
    if (result == true && a.evtblk.strarg) {
        if (std::strcmp(a.evtblk.strarg, b.evtblk.strarg) < 0) {
            result = true;
        } else {
            result = false;
        }
    }
    return result;
}

// Identifiers are always "sourcename:outletname" and "sinkname:inletname",
// or "sourcename:idname:outletname" and "sinkname:inletname."

std::map<CSOUND *, std::map< std::string, std::vector< Outleta * > > > aoutletsForCsoundsForSourceOutletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Outletk * > > > koutletsForCsoundsForSourceOutletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Outletf * > > > foutletsForCsoundsForSourceOutletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Outletv * > > > voutletsForCsoundsForSourceOutletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Outletkid * > > > kidoutletsForCsoundsForSourceOutletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Inleta * > > > ainletsForCsoundsForSinkInletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Inletk * > > > kinletsForCsoundsForSinkInletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Inletf * > > > finletsForCsoundsForSinkInletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Inletv * > > > vinletsForCsoundsForSinkInletIds;
std::map<CSOUND *, std::map< std::string, std::vector< Inletkid * > > > kidinletsForCsoundsForSinkInletIds;
std::map<CSOUND *, std::map< std::string, std::vector< std::string > > > connectionsForCsounds;
std::map<CSOUND *, std::map< EventBlock, int > > functionTablesForCsoundsForEvtblks;
std::map<CSOUND *, std::vector< std::vector< std::vector<Outleta *> *> * > > aoutletVectorsForCsounds;
std::map<CSOUND *, std::vector< std::vector< std::vector<Outletk *> *> * > > koutletVectorsForCsounds;
std::map<CSOUND *, std::vector< std::vector< std::vector<Outletf *> *> * > > foutletVectorsForCsounds;
std::map<CSOUND *, std::vector< std::vector< std::vector<Outletv *> *> * > > voutletVectorsForCsounds;
std::map<CSOUND *, std::vector< std::vector< std::vector<Outletkid *> *> * > > kidoutletVectorsForCsounds;

// For true thread-safety, access to shared data must be protected.
// We will use one OpenMP critical section for each logically independent
// potential data race here: ports and ftables.

/**
 * All it does is clear the data structures for the current instance of Csound,
 * in case they are full from a previous performance.
 */
struct SignalFlowGraph : public OpcodeBase<SignalFlowGraph> {
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      aoutletsForCsoundsForSourceOutletIds[csound].clear();
      ainletsForCsoundsForSinkInletIds[csound].clear();
      koutletsForCsoundsForSourceOutletIds[csound].clear();
      kinletsForCsoundsForSinkInletIds[csound].clear();
      foutletsForCsoundsForSourceOutletIds[csound].clear();
      finletsForCsoundsForSinkInletIds[csound].clear();
      voutletsForCsoundsForSourceOutletIds[csound].clear();
      vinletsForCsoundsForSinkInletIds[csound].clear();
      kidoutletsForCsoundsForSourceOutletIds[csound].clear();
      kidinletsForCsoundsForSinkInletIds[csound].clear();
      connectionsForCsounds[csound].clear();
    }
    csound->UnlockMutex(cs_sfg_ports);
//#pragma omp critical (cs_sfg_ftables)
    csound->LockMutex(cs_sfg_ftables);
    {
      functionTablesForCsoundsForEvtblks[csound].clear();
    }
    csound->UnlockMutex(cs_sfg_ftables);
    return OK;
  };
};

struct Outleta : public OpcodeBase<Outleta> {
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  MYFLT *asignal;
  /**
   * State.
   */
  char sourceOutletId[0x100];
  int init(CSOUND *csound) {
    //warn(csound, "BEGAN Outleta::init()...\n");
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      sourceOutletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Outleta *> &aoutlets =
        aoutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
      if (std::find(aoutlets.begin(), aoutlets.end(), this) == aoutlets.end()) {
        aoutlets.push_back(this);
        warn(csound, "Created instance 0x%x of %d instances of outlet %s\n",
             this, aoutlets.size(), sourceOutletId);
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    //warn(csound, "ENDED Outleta::init()...\n");
    return OK;
  }
};

struct Inleta : public OpcodeBase<Inleta> {
  /**
   * Output.
   */
  MYFLT *asignal;
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  /**
   * State.
   */
  char sinkInletId[0x100];
  std::vector< std::vector<Outleta *> *> *sourceOutlets;
  int sampleN;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      warn(csound, "BEGAN Inleta::init()...\n");
      sampleN = opds.insdshead->ksmps;
      warn(csound, "sourceOutlets: 0x%x\n", sourceOutlets);
      if (std::find(aoutletVectorsForCsounds[csound].begin(),
                    aoutletVectorsForCsounds[csound].end(),
                    sourceOutlets) == aoutletVectorsForCsounds[csound].end()) {
        sourceOutlets = new std::vector< std::vector<Outleta *> *>;
        aoutletVectorsForCsounds[csound].push_back(sourceOutlets);
      }
      warn(csound, "sourceOutlets: 0x%x\n", sourceOutlets);
      sinkInletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sinkInletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sinkInletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Inleta *> &ainlets =
        ainletsForCsoundsForSinkInletIds[csound][sinkInletId];
      if (std::find(ainlets.begin(), ainlets.end(), this) == ainlets.end()) {
        ainlets.push_back(this);
        warn(csound, "Created instance 0x%x of inlet %s\n", this, sinkInletId);
      }
      // Find source outlets connecting to this.
      // Any number of sources may connect to any number of sinks.
      std::vector<std::string> &sourceOutletIds =
        connectionsForCsounds[csound][sinkInletId];
      for (size_t i = 0, n = sourceOutletIds.size(); i < n; i++) {
        const std::string &sourceOutletId = sourceOutletIds[i];
        std::vector<Outleta *> &aoutlets =
          aoutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
        if (std::find(sourceOutlets->begin(), sourceOutlets->end(),
                      &aoutlets) == sourceOutlets->end()) {
          sourceOutlets->push_back(&aoutlets);
          warn(csound,
               Str("Connected instances of outlet %s to instance 0x%x of "
                   "inlet %s.\n"), sourceOutletId.c_str(), this, sinkInletId);
        }
      }
      warn(csound, "ENDED Inleta::init().\n");
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
  /**
   * Sum arate values from active outlets feeding this inlet.
   */
  int audio(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      //warn(csound, "BEGAN Inleta::audio()...\n");
      // Zero the inlet buffer.
      for (int sampleI = 0; sampleI < sampleN; sampleI++) {
        asignal[sampleI] = FL(0.0);
      }
      // Loop over the source connections...
      for (size_t sourceI = 0, sourceN = sourceOutlets->size();
           sourceI < sourceN;
           sourceI++) {
        // Loop over the source connection instances...
        std::vector<Outleta *> *instances = sourceOutlets->at(sourceI);
        for (size_t instanceI = 0, instanceN = instances->size();
             instanceI < instanceN;
             instanceI++) {
          Outleta *sourceOutlet = instances->at(instanceI);
          // Skip inactive instances.
          if (sourceOutlet->opds.insdshead->actflg) {
            for (int sampleI = 0, sampleN = ksmps(); sampleI < sampleN; ++sampleI) {
              asignal[sampleI] += sourceOutlet->asignal[sampleI];
            }
          }
        }
      }
      //warn(csound, "ENDED Inleta::audio().\n");
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Outletk : public OpcodeBase<Outletk> {
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  MYFLT *ksignal;
  /**
   * State.
   */
  char sourceOutletId[0x100];
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Outletk *> &koutlets =
        koutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
      if (std::find(koutlets.begin(), koutlets.end(), this) == koutlets.end()) {
        koutlets.push_back(this);
        warn(csound, Str("Created instance 0x%x of %d instances of outlet %s\n"),
             this, koutlets.size(), sourceOutletId);
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Inletk : public OpcodeBase<Inletk> {
  /**
   * Output.
   */
  MYFLT *ksignal;
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  /**
   * State.
   */
  char sinkInletId[0x100];
  std::vector< std::vector<Outletk *> *> *sourceOutlets;
  int ksmps;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {

      ksmps = opds.insdshead->ksmps;
      if (std::find(koutletVectorsForCsounds[csound].begin(),
                    koutletVectorsForCsounds[csound].end(),
                    sourceOutlets) == koutletVectorsForCsounds[csound].end()) {
        sourceOutlets = new std::vector< std::vector<Outletk *> *>;
        koutletVectorsForCsounds[csound].push_back(sourceOutlets);
      }
      sinkInletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sinkInletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sinkInletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Inletk *> &kinlets =
        kinletsForCsoundsForSinkInletIds[csound][sinkInletId];
      if (std::find(kinlets.begin(), kinlets.end(), this) == kinlets.end()) {
        kinlets.push_back(this);
        warn(csound, "Created instance 0x%x of inlet %s\n", this, sinkInletId);
      }
      // Find source outlets connecting to this.
      // Any number of sources may connect to any number of sinks.
      std::vector<std::string> &sourceOutletIds =
        connectionsForCsounds[csound][sinkInletId];
      for (size_t i = 0, n = sourceOutletIds.size(); i < n; i++) {
        const std::string &sourceOutletId = sourceOutletIds[i];
        std::vector<Outletk *> &koutlets =
          koutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
        if (std::find(sourceOutlets->begin(),
                      sourceOutlets->end(), &koutlets) == sourceOutlets->end()) {
          sourceOutlets->push_back(&koutlets);
          warn(csound, Str("Connected instances of outlet %s to instance 0x%x"
                           "of inlet %s.\n"),
               sourceOutletId.c_str(), this, sinkInletId);
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
  /**
   * Sum krate values from active outlets feeding this inlet.
   */
  int kontrol(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    {
      // Zero the inlet buffer.
      *ksignal = FL(0.0);
      // Loop over the source connections...
      for (size_t sourceI = 0, sourceN = sourceOutlets->size();
           sourceI < sourceN;
           sourceI++) {
        // Loop over the source connection instances...
        const std::vector<Outletk *> *instances = sourceOutlets->at(sourceI);
        for (size_t instanceI = 0, instanceN = instances->size();
             instanceI < instanceN;
             instanceI++) {
          const Outletk *sourceOutlet = instances->at(instanceI);
          // Skip inactive instances.
          if (sourceOutlet->opds.insdshead->actflg) {
            *ksignal += *sourceOutlet->ksignal;
          }
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Outletf : public OpcodeBase<Outletf> {
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  PVSDAT *fsignal;
  /**
   * State.
   */
  char sourceOutletId[0x100];
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    {
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Outletf *> &foutlets =
        foutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
      if (std::find(foutlets.begin(), foutlets.end(), this) == foutlets.end()) {
        foutlets.push_back(this);
        warn(csound, "Created instance 0x%x of outlet %s\n", this, sourceOutletId);
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Inletf : public OpcodeBase<Inletf> {
  /**
   * Output.
   */
  PVSDAT *fsignal;
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  /**
   * State.
   */
  char sinkInletId[0x100];
  std::vector< std::vector<Outletf *> *> *sourceOutlets;
  int ksmps;
  int lastframe;
  bool fsignalInitialized;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      ksmps = opds.insdshead->ksmps;
      lastframe = 0;
      fsignalInitialized = false;
      if (std::find(foutletVectorsForCsounds[csound].begin(),
                    foutletVectorsForCsounds[csound].end(),
                    sourceOutlets) == foutletVectorsForCsounds[csound].end()) {
        sourceOutlets = new std::vector< std::vector<Outletf *> *>;
        foutletVectorsForCsounds[csound].push_back(sourceOutlets);
      }
      sinkInletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sinkInletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sinkInletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Inletf *> &finlets =
        finletsForCsoundsForSinkInletIds[csound][sinkInletId];
      if (std::find(finlets.begin(), finlets.end(), this) == finlets.end()) {
        finlets.push_back(this);
        warn(csound, "Created instance 0x%x of inlet %s\n", this, sinkInletId);
      }
      // Find source outlets connecting to this.
      // Any number of sources may connect to any number of sinks.
      std::vector<std::string> &sourceOutletIds =
        connectionsForCsounds[csound][sinkInletId];
      for (size_t i = 0, n = sourceOutletIds.size(); i < n; i++) {
        const std::string &sourceOutletId = sourceOutletIds[i];
        std::vector<Outletf *> &foutlets =
          foutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
        if (std::find(sourceOutlets->begin(),
                      sourceOutlets->end(), &foutlets) == sourceOutlets->end()) {
          sourceOutlets->push_back(&foutlets);
          warn(csound,
               Str("Connected instances of outlet %s to instance 0x%x of inlet %s.\n"),
               sourceOutletId.c_str(), this, sinkInletId);
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
  /**
   * Mix fsig values from active outlets feeding this inlet.
   */
  int audio(CSOUND *csound) {
    int result = OK;
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      float *sink = 0;
      float *source = 0;
      CMPLX *sinkFrame = 0;
      CMPLX *sourceFrame = 0;
      // Loop over the source connections...
      for (size_t sourceI = 0, sourceN = sourceOutlets->size();
           sourceI < sourceN;
           sourceI++) {
        // Loop over the source connection instances...
        const std::vector<Outletf *> *instances = sourceOutlets->at(sourceI);
        for (size_t instanceI = 0, instanceN = instances->size();
             instanceI < instanceN;
             instanceI++) {
          const Outletf *sourceOutlet = instances->at(instanceI);
          // Skip inactive instances.
          if (sourceOutlet->opds.insdshead->actflg) {
            if (!fsignalInitialized) {
              int32 N = sourceOutlet->fsignal->N;
              if (UNLIKELY(sourceOutlet->fsignal == fsignal)) {
                csound->Warning(csound,
                                Str("Unsafe to have same fsig as in and out"));
              }
              fsignal->sliding = 0;
              if (sourceOutlet->fsignal->sliding) {
                if (fsignal->frame.auxp == NULL ||
                    fsignal->frame.size <
                    sizeof(MYFLT) * opds.insdshead->ksmps * (N + 2))
                  csound->AuxAlloc(csound,
                                   (N + 2) * sizeof(MYFLT) * opds.insdshead->ksmps,
                                   &fsignal->frame);
                fsignal->NB = sourceOutlet->fsignal->NB;
                fsignal->sliding = 1;
              } else
                if (fsignal->frame.auxp == NULL ||
                    fsignal->frame.size < sizeof(float) * (N + 2)) {
                  csound->AuxAlloc(csound,
                                   (N + 2) * sizeof(float), &fsignal->frame);
                }
              fsignal->N = N;
              fsignal->overlap = sourceOutlet->fsignal->overlap;
              fsignal->winsize = sourceOutlet->fsignal->winsize;
              fsignal->wintype = sourceOutlet->fsignal->wintype;
              fsignal->format = sourceOutlet->fsignal->format;
              fsignal->framecount = 1;
              lastframe = 0;
              if (UNLIKELY(!(fsignal->format == PVS_AMP_FREQ) ||
                           (fsignal->format == PVS_AMP_PHASE)))
                result =
                  csound->InitError(csound, Str("inletf: signal format "
                                                "must be amp-phase or amp-freq."));
              fsignalInitialized = true;
            }
            if (fsignal->sliding) {
              for (int frameI = 0; frameI < ksmps; frameI++) {
                sinkFrame = (CMPLX*) fsignal->frame.auxp + (fsignal->NB * frameI);
                sourceFrame =
                  (CMPLX*) sourceOutlet->fsignal->frame.auxp + (fsignal->NB * frameI);
                for (size_t binI = 0, binN = fsignal->NB; binI < binN; binI++) {
                  if (sourceFrame[binI].re > sinkFrame[binI].re) {
                    sinkFrame[binI] = sourceFrame[binI];
                  }
                }
              }
            }
          } else {
            sink = (float *)fsignal->frame.auxp;
            source = (float *)sourceOutlet->fsignal->frame.auxp;
            if (lastframe < int(fsignal->framecount)) {
              for (size_t binI = 0, binN = fsignal->N + 2;
                   binI < binN;
                   binI += 2) {
                if (source[binI] > sink[binI]) {
                  source[binI] = sink[binI];
                  source[binI + 1] = sink[binI + 1];
                }
              }
              fsignal->framecount = lastframe = sourceOutlet->fsignal->framecount;
            }
          }
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return result;
  }
};

struct Outletv : public OpcodeBase<Outletv> {
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  ARRAYDAT *vsignal;
  /**
   * State.
   */
  char sourceOutletId[0x100];
  int init(CSOUND *csound) {
    warn(csound, "BEGAN Outletv::init()...\n");
//#pragma omp critical (cs_sfg_ports)
    csound->UnlockMutex(cs_sfg_ports);
    {
      sourceOutletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Outletv *> &voutlets =
        voutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
      if (std::find(voutlets.begin(), voutlets.end(), this) == voutlets.end()) {
        voutlets.push_back(this);
        warn(csound, "Created instance 0x%x of %d instances of outlet %s (out arraydat: 0x%x dims: %2d size: %4d [%4d] data: 0x%x (0x%x))\n",
             this, voutlets.size(), sourceOutletId, vsignal, vsignal->dimensions, vsignal->sizes[0], vsignal->arrayMemberSize, vsignal->data, &vsignal->data);
      }
    }
    warn(csound, "ENDED Outletv::init()...\n");
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Inletv : public OpcodeBase<Inletv> {
  /**
   * Output.
   */
  ARRAYDAT *vsignal;
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  /**
   * State.
   */
  char sinkInletId[0x100];
  std::vector< std::vector<Outletv *> *> *sourceOutlets;
  size_t arraySize;
  size_t myFltsPerArrayElement;
  int sampleN;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      warn(csound, "BEGAN Inletv::init()...\n");
      sampleN = opds.insdshead->ksmps;
      // The array elements may be krate (1 MYFLT) or arate (ksmps MYFLT).
      myFltsPerArrayElement = vsignal->arrayMemberSize / sizeof(MYFLT);
      warn(csound, "myFltsPerArrayElement: %d\n", myFltsPerArrayElement);
      arraySize = myFltsPerArrayElement;
      for(size_t dimension = 0; dimension < vsignal->dimensions; ++dimension) {
          arraySize *= MYFLT2LRND(vsignal->sizes[dimension]);
      }
      warn(csound, "arraySize: %d\n", arraySize);
      warn(csound, "sourceOutlets: 0x%x\n", sourceOutlets);
      if (std::find(voutletVectorsForCsounds[csound].begin(),
                    voutletVectorsForCsounds[csound].end(),
                    sourceOutlets) == voutletVectorsForCsounds[csound].end()) {
        sourceOutlets = new std::vector< std::vector<Outletv *> *>;
        voutletVectorsForCsounds[csound].push_back(sourceOutlets);
      }
      warn(csound, "sourceOutlets: 0x%x\n", sourceOutlets);
      sinkInletId[0] = 0;
      const char *insname =
        csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sinkInletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sinkInletId, "%d:%s", opds.insdshead->insno,
                     (char *)Sname->data);
      }
      std::vector<Inletv *> &vinlets =
        vinletsForCsoundsForSinkInletIds[csound][sinkInletId];
      if (std::find(vinlets.begin(), vinlets.end(), this) == vinlets.end()) {
        vinlets.push_back(this);
        warn(csound, "Created instance 0x%x of inlet %s (in arraydat: 0x%x dims: %2d size: %4d [%4d] data: 0x%x (0x%x))\n",
             this, sinkInletId, vsignal, vsignal->dimensions, vsignal->sizes[0], vsignal->arrayMemberSize, vsignal->data, &vsignal->data);
      }
      // Find source outlets connecting to this.
      // Any number of sources may connect to any number of sinks.
      std::vector<std::string> &sourceOutletIds =
        connectionsForCsounds[csound][sinkInletId];
      for (size_t i = 0, n = sourceOutletIds.size(); i < n; i++) {
        const std::string &sourceOutletId = sourceOutletIds[i];
        std::vector<Outletv*> &voutlets =
          voutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
        if (std::find(sourceOutlets->begin(), sourceOutlets->end(),
                      &voutlets) == sourceOutlets->end()) {
          sourceOutlets->push_back(&voutlets);
          warn(csound,
               Str("Connected instances of outlet %s to instance 0x%x of "
                   "inlet %s\n"), sourceOutletId.c_str(), this, sinkInletId);
        }
      }
      warn(csound, "ENDED Inletv::init().\n");
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
  /**
   * Sum values from active outlets feeding this inlet.
   */
  int audio(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      //warn(csound, "BEGAN Inletv::audio()...\n");
      for (uint32_t signalI = 0; signalI < arraySize; ++signalI) {
        vsignal->data[signalI] = FL(0.0);
      }
      // Loop over the source connections...
      for (size_t sourceI = 0, sourceN = sourceOutlets->size();
           sourceI < sourceN;
           sourceI++) {
        // Loop over the source connection instances...
        std::vector<Outletv *> *instances = sourceOutlets->at(sourceI);
        for (size_t instanceI = 0, instanceN = instances->size();
             instanceI < instanceN;
             instanceI++) {
          Outletv *sourceOutlet = instances->at(instanceI);
          // Skip inactive instances.
          if (sourceOutlet->opds.insdshead->actflg) {
            for (uint32_t signalI = 0; signalI < arraySize; ++signalI) {
                ARRAYDAT *insignal = sourceOutlet->vsignal;
                MYFLT *indata = insignal->data;
                //warn(csound, "Inletv::audio: sourceOutlet: 0%x in arraydat: 0x%x data: 0x%x (0x%x)\n", sourceOutlet, insignal, indata, &insignal->data);
                vsignal->data[signalI] += indata[signalI];
            }
          }
        }
      }
      //warn(csound, "ENDED Inletv::audio().\n");
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Outletkid : public OpcodeBase<Outletkid> {
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  STRINGDAT *SinstanceId;
  MYFLT *ksignal;
  /**
   * State.
   */
  char sourceOutletId[0x100];
  char *instanceId;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      const char *insname = csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      instanceId = csound->strarg2name(csound,
                   (char*) NULL,
                   SinstanceId->data,
                   (char *)"",
                   1);
      if (insname && instanceId) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno, (char *)Sname->data);
      }
      if (insname) {
        std::sprintf(sourceOutletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sourceOutletId, "%d:%s", opds.insdshead->insno, (char *)Sname->data);
      }
      std::vector<Outletkid *> &koutlets = kidoutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
      if (std::find(koutlets.begin(), koutlets.end(), this) == koutlets.end()) {
        koutlets.push_back(this);
        warn(csound, "Created instance 0x%x of %d instances of outlet %s\n", this, koutlets.size(), sourceOutletId);
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Inletkid : public OpcodeBase<Inletkid> {
  /**
   * Output.
   */
  MYFLT *ksignal;
  /**
   * Inputs.
   */
  STRINGDAT *Sname;
  STRINGDAT *SinstanceId;
  /**
   * State.
   */
  char sinkInletId[0x100];
  char *instanceId;
  std::vector< std::vector<Outletkid *> *> *sourceOutlets;
  int ksmps;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      ksmps = opds.insdshead->ksmps;
      if (std::find(kidoutletVectorsForCsounds[csound].begin(),
                    kidoutletVectorsForCsounds[csound].end(),
                    sourceOutlets) == kidoutletVectorsForCsounds[csound].end()) {
        sourceOutlets = new std::vector< std::vector<Outletkid *> *>;
        kidoutletVectorsForCsounds[csound].push_back(sourceOutlets);
      }
      sinkInletId[0] = 0;
      instanceId = csound->strarg2name(csound,
                   (char*) NULL,
                   SinstanceId->data,
                   (char *)"",
                   1);
      const char *insname = csound->GetInstrumentList(csound)[opds.insdshead->insno]->insname;
      if (insname) {
        std::sprintf(sinkInletId, "%s:%s", insname, (char *)Sname->data);
      } else {
        std::sprintf(sinkInletId, "%d:%s", opds.insdshead->insno, (char *)Sname->data);
      }
      std::vector<Inletkid *> &kinlets = kidinletsForCsoundsForSinkInletIds[csound][sinkInletId];
      if (std::find(kinlets.begin(), kinlets.end(), this) == kinlets.end()) {
        kinlets.push_back(this);
        warn(csound, "Created instance 0x%x of inlet %s\n", this, sinkInletId);
      }
      // Find source outlets connecting to this.
      // Any number of sources may connect to any number of sinks.
      std::vector<std::string> &sourceOutletIds = connectionsForCsounds[csound][sinkInletId];
      for (size_t i = 0, n = sourceOutletIds.size(); i < n; i++) {
        const std::string &sourceOutletId = sourceOutletIds[i];
        std::vector<Outletkid *> &koutlets = kidoutletsForCsoundsForSourceOutletIds[csound][sourceOutletId];
        if (std::find(sourceOutlets->begin(), sourceOutlets->end(), &koutlets) == sourceOutlets->end()) {
          sourceOutlets->push_back(&koutlets);
          warn(csound, "Connected instances of outlet %s to instance 0x%x of inlet %s.\n", sourceOutletId.c_str(), this, sinkInletId);
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
  /**
   * Replay instance signal.
   */
  int kontrol(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      // Zero the / buffer.
      *ksignal = FL(0.0);
      // Loop over the source connections...
      for (size_t sourceI = 0, sourceN = sourceOutlets->size();
           sourceI < sourceN;
           sourceI++) {
        // Loop over the source connection instances...
        const std::vector<Outletkid *> *instances = sourceOutlets->at(sourceI);
        for (size_t instanceI = 0, instanceN = instances->size();
             instanceI < instanceN;
             instanceI++) {
          const Outletkid *sourceOutlet = instances->at(instanceI);
          // Skip inactive instances and also all non-matching instances.
          if (sourceOutlet->opds.insdshead->actflg) {
        if (std::strcmp(sourceOutlet->instanceId, instanceId) == 0) {
            *ksignal += *sourceOutlet->ksignal;
        }
          }
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Connect : public OpcodeBase<Connect> {
  /**
   * Inputs.
   */
  MYFLT *Source;
  STRINGDAT *Soutlet;
  MYFLT  *Sink;
  STRINGDAT *Sinlet;
  MYFLT *gain;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      std::string sourceOutletId = csound->strarg2name(csound,
                                                       (char *) 0,
                                                       ((std::isnan(*Source)) ?
                                                       csound->GetString(csound,*Source) :
                                                        (char *)Source),
                                                       (char *)"",
                                                       std::isnan(*Source));
      sourceOutletId += ":";
      sourceOutletId += csound->strarg2name(csound,
                                            (char *) 0,
                                            Soutlet->data,
                                            (char *)"",
                                            1);

      std::string sinkInletId = csound->strarg2name(csound,
                                                    (char *) 0,
                                                    ((std::isnan(*Sink)) ?
                                                       csound->GetString(csound,*Sink) :
                                                        (char *)Sink),
                                                       (char *)"",
                                                       std::isnan(*Sink));
      sinkInletId += ":";
      sinkInletId += csound->strarg2name(csound,
                                         (char *) 0,
                                         Sinlet->data,
                                         (char *)"",
                                         1);
      warn(csound, "Connected outlet %s to inlet %s.\n", sourceOutletId.c_str(), sinkInletId.c_str());
      connectionsForCsounds[csound][sinkInletId].push_back(sourceOutletId);
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct Connecti : public OpcodeBase<Connecti> {
  /**
   * Inputs.
   */
  MYFLT *Source;
  STRINGDAT *Soutlet;
  STRINGDAT *Sink;
  STRINGDAT *Sinlet;
  MYFLT *gain;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      std::string sourceOutletId = csound->strarg2name(csound,
                                                       (char *) 0,
                                                       ((std::isnan(*Source)) ?
                                                       csound->GetString(csound,*Source) :
                                                        (char *)Source),
                                                       (char *)"",
                                                       std::isnan(*Source));
      sourceOutletId += ":";
      sourceOutletId += csound->strarg2name(csound,
                                            (char *) 0,
                                            Soutlet->data,
                                            (char *)"",
                                            1);
      std::string sinkInletId = csound->strarg2name(csound,
                                                    (char *) 0,
                                                    Sink->data,
                                                    (char *)"",
                                                    1);
      sinkInletId += ":";
      sinkInletId += csound->strarg2name(csound,
                                         (char *) 0,
                                         Sinlet->data,
                                         (char *)"",
                                         1);
      warn(csound, "Connected outlet %s to inlet %s.\n", sourceOutletId.c_str(), sinkInletId.c_str());
      connectionsForCsounds[csound][sinkInletId].push_back(sourceOutletId);
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};


struct Connectii : public OpcodeBase<Connectii> {
  /**
   * Inputs.
   */
  STRINGDAT *Source;
  STRINGDAT *Soutlet;
  MYFLT *Sink;
  STRINGDAT *Sinlet;
  MYFLT *gain;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      std::string sourceOutletId = csound->strarg2name(csound,
                                                       (char *) 0,
                                                       Source->data,
                                                       (char *)"",
                                                       1);
      sourceOutletId += ":";
      sourceOutletId += csound->strarg2name(csound,
                                            (char *) 0,
                                            Soutlet->data,
                                            (char *)"",
                                            1);
      std::string sinkInletId =
        csound->strarg2name(csound,
                            (char *) 0,
                            ((std::isnan(*Sink)) ?
                             csound->GetString(csound,*Sink) :
                             (char *)Sink),
                            (char *)"",
                            std::isnan(*Sink));;
      sinkInletId += ":";
      sinkInletId += csound->strarg2name(csound,
                                         (char *) 0,
                                         Sinlet->data,
                                         (char *)"",
                                         1);
      warn(csound, Str("Connected outlet %s to inlet %s.\n"),
           sourceOutletId.c_str(), sinkInletId.c_str());
      connectionsForCsounds[csound][sinkInletId].push_back(sourceOutletId);
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct ConnectS : public OpcodeBase<ConnectS> {
  /**
   * Inputs.
   */
  STRINGDAT *Source;
  STRINGDAT *Soutlet;
  STRINGDAT *Sink;
  STRINGDAT *Sinlet;
  MYFLT *gain;
  int init(CSOUND *csound) {
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
      std::string sourceOutletId = csound->strarg2name(csound,
                                                       (char *) 0,
                                                       Source->data,
                                                       (char *)"",
                                                       1);
      sourceOutletId += ":";
      sourceOutletId += csound->strarg2name(csound,
                                            (char *) 0,
                                            Soutlet->data,
                                            (char *)"",
                                            1);
      std::string sinkInletId = csound->strarg2name(csound,
                                                    (char *) 0,
                                                    Sink->data,
                                                    (char *)"",
                                                    1);
      sinkInletId += ":";
      sinkInletId += csound->strarg2name(csound,
                                         (char *) 0,
                                         Sinlet->data,
                                         (char *)"",
                                         1);
      warn(csound, Str("Connected outlet %s to inlet %s.\n"),
           sourceOutletId.c_str(), sinkInletId.c_str());
      connectionsForCsounds[csound][sinkInletId].push_back(sourceOutletId);
    }
    csound->UnlockMutex(cs_sfg_ports);
    return OK;
  }
};

struct AlwaysOnS  : public OpcodeBase<AlwaysOnS> {
  /**
   * Inputs.
   */
  STRINGDAT *Sinstrument;
  MYFLT *argums[VARGMAX];
  /**
   * State.
   */
  EVTBLK evtblk;
  int init(CSOUND *csound) {
    evtblk.opcod = 'i';
    evtblk.strarg = NULL;
    evtblk.p[0] = FL(0.0);
    evtblk.p[1] = csound->strarg2insno(csound, Sinstrument->data, 1);
    evtblk.p[2] = evtblk.p2orig = FL(0.0);
    evtblk.p[3] = evtblk.p3orig = FL(-1.0);
    size_t inArgCount = csound->GetInputArgCnt(this);
    // Add 2, for hard-coded p2 and p3.
    evtblk.pcnt = (int16) inArgCount + 2;
    // Subtract 1, for only required inarg p1.
    size_t argumN = inArgCount - 1;
    // Start evtblk at 4, argums at 0.
    for (size_t pfieldI = 4, argumI = 0; argumI < argumN; pfieldI++, argumI++) {
      evtblk.p[pfieldI] = *argums[argumI];
    }
    csound->insert_score_event(csound, &evtblk, FL(0.0));
    return OK;
  }
};

struct AlwaysOn  : public OpcodeBase<AlwaysOn> {
  /**
   * Inputs.
   */
  MYFLT *Sinstrument;
  MYFLT *argums[VARGMAX];
  /**
   * State.
   */
  EVTBLK evtblk;
  int init(CSOUND *csound) {
    std::string source = csound->strarg2name(csound,
                                             (char *) 0,
                                             Sinstrument,
                                             (char *)"",
                                             (int) 0);
    evtblk.opcod = 'i';
    evtblk.strarg = NULL;
    evtblk.p[0] = FL(0.0);
    evtblk.p[1] = *Sinstrument;
    evtblk.p[2] = evtblk.p2orig = FL(0.0);
    evtblk.p[3] = evtblk.p3orig = FL(-1.0);

    size_t inArgCount = csound->GetInputArgCnt(this);
    // Add 2, for hard-coded p2 and p3.
    evtblk.pcnt = (int16) inArgCount + 2;
    // Subtract 1, for only required inarg p1.
    size_t argumN = inArgCount - 1;
    // Start evtblk at 4, argums at 0.
    for (size_t pfieldI = 4, argumI = 0; argumI < argumN; pfieldI++, argumI++) {
      evtblk.p[pfieldI] = *argums[argumI];
    }
    csound->insert_score_event_at_sample(csound, &evtblk, 0);
    return OK;
  }
};

struct FtGenOnce : public OpcodeBase<FtGenOnce> {
  /**
   * Outputs.
   */
  MYFLT *ifno;
  /**
   * Inputs.
   */
  MYFLT *p1;
  MYFLT *p2;
  MYFLT *p3;
  MYFLT *p4;
  MYFLT *p5;
  MYFLT *argums[VARGMAX];
  EventBlock eventBlock;
  int init(CSOUND *csound) {
    int result = OK;
//#pragma omp critical (cs_ftables)
    csound->LockMutex(cs_sfg_ftables);
    {
      // Default output.
      *ifno = FL(0.0);
      EVTBLK &evtblk = eventBlock.evtblk;
      std::memset(&evtblk, 0, sizeof(EVTBLK));
      // ifno ftgenonce ipfno, ip2dummy, ip4size, ip5gen, ip6arga, ip7argb,...
      evtblk.opcod = 'f';
      evtblk.strarg = 0;
      evtblk.p[0] = FL(0.0);
      evtblk.p[1] = *p1;
      evtblk.p[2] = evtblk.p2orig = FL(0.0);
      evtblk.p[3] = evtblk.p3orig = *p3;
      evtblk.p[4] = *p4;
      evtblk.p[5] = *p5;
      evtblk.pcnt = (int16) csound->GetInputArgCnt(this);
      int n = evtblk.pcnt - 5;
      if (n > 0) {
          MYFLT **argp = argums;
          MYFLT *fp = &evtblk.p[0] + 6;
          do {
            *fp++ = **argp++;
          } while (--n);
        }
        // If the arguments have not been used before for this instance of Csound,
        // create a new function table and store the arguments and table number;
        // otherwise, look up and return the already created function table's number.
        if(functionTablesForCsoundsForEvtblks[csound].find(eventBlock) != functionTablesForCsoundsForEvtblks[csound].end()) {
          *ifno = functionTablesForCsoundsForEvtblks[csound][eventBlock];
          warn(csound, "ftgenonce: re-using existing func: %f\n", *ifno);
          // std::cerr << "ftgenonce: re-using existing func:" << evtblk << std::endl;
        } else {
          FUNC *func = 0;
          int status = csound->hfgens(csound, &func, &evtblk, 1);
          if (UNLIKELY(status != 0)) {
            result = csound->InitError(csound, Str("ftgenonce error"));
          }
          if (func) {
            functionTablesForCsoundsForEvtblks[csound][eventBlock] = func->fno;
            *ifno = (MYFLT) func->fno;
            warn(csound, "ftgenonce: created new func: %d\n", func->fno);
            // std::cerr << "ftgenonce: created new func:" << evtblk << std::endl;
            if(functionTablesForCsoundsForEvtblks[csound].find(eventBlock) == functionTablesForCsoundsForEvtblks[csound].end()) {
#if (SIGNALFLOWGRAPH_DEBUG == 1)
                std::fprintf(stderr, "Oops! inserted but not found.\n");
#endif
            }
          } else {
#if (SIGNALFLOWGRAPH_DEBUG == 1)
            std::fprintf(stderr, "Oops! New but not created.\n");
#endif
          }
        }
    }
    csound->UnlockMutex(cs_sfg_ftables);
    return result;
  }
};

struct FtGenOnceS : public OpcodeBase<FtGenOnceS> {
  /**
   * Outputs.
   */
  MYFLT *ifno;
  /**
   * Inputs.
   */
  MYFLT *p1;
  MYFLT *p2;
  MYFLT *p3;
  MYFLT *p4;
  MYFLT *p5;
  MYFLT *argums[VARGMAX];
  EventBlock eventBlock;
  int init(CSOUND *csound) {
    int result = OK;
//#pragma omp critical (cs_ftables)
    csound->LockMutex(cs_sfg_ftables);
    {
      // Default output.
      *ifno = FL(0.0);
      EVTBLK &evtblk = eventBlock.evtblk;
      std::memset(&evtblk, 0, sizeof(EVTBLK));
      evtblk.opcod = 'f';
      evtblk.strarg = 0;
      evtblk.p[0] = FL(0.0);
      evtblk.p[1] = *p1;
      evtblk.p[2] = evtblk.p2orig = FL(0.0);
      evtblk.p[3] = evtblk.p3orig = *p3;
      evtblk.p[4] = *p4;
      evtblk.p[5] = SSTRCOD;
      int gen = (int) std::fabs(evtblk.p[4]);
      // Only GEN 1, 23, 28, or 43 can take strings.
      switch (gen) {
        case 1:
        case 23:
        case 28:
        case 43:
          evtblk.strarg = ((STRINGDAT *)p5)->data;
          break;
        default:
          result = csound->InitError(csound, Str("ftgen string arg not allowed"));
      }
      if (result == OK) {
        evtblk.pcnt = (int16) csound->GetInputArgCnt(this);
        int n = evtblk.pcnt - 5;
        if (n > 0) {
          MYFLT **argp = argums;
          MYFLT *fp = &evtblk.p[0] + 6;
          do {
            *fp++ = **argp++;
          } while (--n);
        }
        // If the arguments have not been used before for this instance of Csound,
        // create a new function table and store the arguments and table number;
        // otherwise, look up and return the already created function table's number.
        if(functionTablesForCsoundsForEvtblks[csound].find(eventBlock) != functionTablesForCsoundsForEvtblks[csound].end()) {
          *ifno = functionTablesForCsoundsForEvtblks[csound][eventBlock];
          warn(csound, "ftgenonce: re-using existing func: %f\n", *ifno);
          // std::cerr << "ftgenonce: re-using existing func:" << evtblk << std::endl;
        } else {
          FUNC *func = 0;
          int status = csound->hfgens(csound, &func, &evtblk, 1);
          if (UNLIKELY(status != 0)) {
            result = csound->InitError(csound, Str("ftgenonce error"));
          }
          if (func) {
            functionTablesForCsoundsForEvtblks[csound][eventBlock] = func->fno;
            *ifno = (MYFLT) func->fno;
            warn(csound, "ftgenonce: created new func: %d\n", func->fno);
            // std::cerr << "ftgenonce: created new func:" << evtblk << std::endl;
          }
        }
      }
    }
    csound->UnlockMutex(cs_sfg_ftables);
    return result;
  }
};


extern "C"
{
  static OENTRY oentries[] = {
    /*    {
          (char *)"signalflowgraph",
          sizeof(SignalFlowGraph),
          0,
          1,
          (char *)"",
          (char *)"",
          (SUBR)&SignalFlowGraph::init_,
          0,
          0,
          }, */
    {
      (char *)"outleta",
      sizeof(Outleta),
      CW,
      5,
      (char *)"",
      (char *)"Sa",
      (SUBR)&Outleta::init_,
      0,
      (SUBR)&Outleta::audio_
    },
    {
      (char *)"inleta",
      sizeof(Inleta),
      CR,
      5,
      (char *)"a",
      (char *)"S",
      (SUBR)&Inleta::init_,
      0,
      (SUBR)&Inleta::audio_
    },
    {
      (char *)"outletk",
      sizeof(Outletk),
      CW,
      3,
      (char *)"",
      (char *)"Sk",
      (SUBR)&Outletk::init_,
      (SUBR)&Outletk::kontrol_,
      0
    },
    {
      (char *)"inletk",
      sizeof(Inletk),
      CR,
      3,
      (char *)"k",
      (char *)"S",
      (SUBR)&Inletk::init_,
      (SUBR)&Inletk::kontrol_,
      0
    },
    {
      (char *)"outletkid",
      sizeof(Outletkid),
      CW,
      3,
      (char *)"",
      (char *)"SSk",
      (SUBR)&Outletk::init_,
      (SUBR)&Outletk::kontrol_,
      0
    },
    {
      (char *)"inletkid",
      sizeof(Inletkid),
      CR,
      3,
      (char *)"k",
      (char *)"SS",
      (SUBR)&Inletk::init_,
      (SUBR)&Inletk::kontrol_,
      0
    },
    {
      (char *)"outletf",
      sizeof(Outletf),
      CW,
      5,
      (char *)"",
      (char *)"Sf",
      (SUBR)&Outletf::init_,
      0,
      (SUBR)&Outletf::audio_
    },
    {
      (char *)"inletf",
      sizeof(Inletf),
      CR,
      5,
      (char *)"f",
      (char *)"S",
      (SUBR)&Inletf::init_,
      0,
      (SUBR)&Inletf::audio_
    },
    {
      (char *)"outletv",
      sizeof(Outletv),
      CW,
      5,
      (char *)"",
      (char *)"Sa[]",
      (SUBR)&Outletv::init_,
      0,
      (SUBR)&Outletv::audio_
    },
    {
      (char *)"inletv",
      sizeof(Inletv),
      CR,
      5,
      (char *)"a[]",
      (char *)"S",
      (SUBR)&Inletv::init_,
      0,
      (SUBR)&Inletv::audio_
    },
    {
      (char *)"connect",
      sizeof(Connect),
      0,
      1,
      (char *)"",
      (char *)"iSiSp",
      (SUBR)&Connect::init_,
      0,
      0
    },
        {
      (char *)"connect.i",
      sizeof(Connecti),
      0,
      1,
      (char *)"",
      (char *)"iSSSp",
      (SUBR)&Connecti::init_,
      0,
      0
    },
        {
      (char *)"connect.ii",
      sizeof(Connectii),
      0,
      1,
      (char *)"",
      (char *)"SSiSp",
      (SUBR)&Connectii::init_,
      0,
      0
    },
        {
      (char *)"connect.S",
      sizeof(ConnectS),
      0,
      1,
      (char *)"",
      (char *)"SSSSp",
      (SUBR)&ConnectS::init_,
      0,
      0
    },
    {
      (char *)"alwayson",
      sizeof(AlwaysOn),
      0,
      1,
      (char *)"",
      (char *)"im",
      (SUBR)&AlwaysOn::init_,
      0,
      0
    },
     {
      (char *)"alwayson.S",
      sizeof(AlwaysOnS),
      0,
      1,
      (char *)"",
      (char *)"Sm",
      (SUBR)&AlwaysOnS::init_,
      0,
      0
    },
    {
      (char *)"ftgenonce",
      sizeof(FtGenOnce),
      TW,
      1,
      (char *)"i",
      (char *)"iiiiim",
      (SUBR)&FtGenOnce::init_,
      0,
      0
    },
     {
      (char *)"ftgenonce.S",
      sizeof(FtGenOnce),
      TW,
      1,
      (char *)"i",
      (char *)"iiiiSm",
      (SUBR)&FtGenOnceS::init_,
      0,
      0
    },
    { 0, 0, 0, 0, 0, 0, (SUBR) 0, (SUBR) 0, (SUBR) 0 }
  };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    if(csound->GetDebug(csound)) {
        csound->Message(csound, "signalflowgraph: csoundModuleCreate(%p)\n", csound);
    }
    if (cs_sfg_ports == 0) {
        cs_sfg_ports = csound->Create_Mutex(1);
    }
    if (cs_sfg_ftables == 0) {
        cs_sfg_ftables = csound->Create_Mutex(1);
    }
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    if(csound->GetDebug(csound)) {
        csound->Message(csound, "signalflowgraph: csoundModuleInit(%p)\n", csound);
    }
    OENTRY *ep = (OENTRY *)&(oentries[0]);
    int  err = 0;
    while (ep->opname != 0) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname,
                                  ep->dsblksiz,
                                  ep->flags,
                                  ep->thread,
                                  ep->outypes,
                                  ep->intypes,
                                  (int (*)(CSOUND *, void*)) ep->iopadr,
                                  (int (*)(CSOUND *, void*)) ep->kopadr,
                                  (int (*)(CSOUND *, void*)) ep->aopadr);
      ep++;
    }
    return err;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    if(csound->GetDebug(csound)) {
        csound->Message(csound, "signalflowgraph: csoundModuleDestroy(%p)\n", csound);
    }
//#pragma omp critical (cs_sfg_ports)
    csound->LockMutex(cs_sfg_ports);
    {
        if (aoutletsForCsoundsForSourceOutletIds.find(csound) != aoutletsForCsoundsForSourceOutletIds.end()) {
            aoutletsForCsoundsForSourceOutletIds[csound].clear();
        }
        if (ainletsForCsoundsForSinkInletIds.find(csound) != ainletsForCsoundsForSinkInletIds.end()) {
            ainletsForCsoundsForSinkInletIds[csound].clear();
        }
        if (aoutletVectorsForCsounds.find(csound) != aoutletVectorsForCsounds.end()) {
            aoutletVectorsForCsounds[csound].clear();
        }
        if (koutletsForCsoundsForSourceOutletIds.find(csound) != koutletsForCsoundsForSourceOutletIds.end()) {
            koutletsForCsoundsForSourceOutletIds[csound].clear();
        }
        if (kinletsForCsoundsForSinkInletIds.find(csound) != kinletsForCsoundsForSinkInletIds.end()) {
            kinletsForCsoundsForSinkInletIds[csound].clear();
        }
        if (koutletVectorsForCsounds.find(csound) != koutletVectorsForCsounds.end()) {
            koutletVectorsForCsounds[csound].clear();
        }
        if (foutletsForCsoundsForSourceOutletIds.find(csound) != foutletsForCsoundsForSourceOutletIds.end()) {
            foutletsForCsoundsForSourceOutletIds[csound].clear();
        }
        if (finletsForCsoundsForSinkInletIds.find(csound) != finletsForCsoundsForSinkInletIds.end()) {
            finletsForCsoundsForSinkInletIds[csound].clear();
        }
        if (foutletVectorsForCsounds.find(csound) != foutletVectorsForCsounds.end()) {
            foutletVectorsForCsounds[csound].clear();
        }
        if (connectionsForCsounds.find(csound) != connectionsForCsounds.end()) {
            connectionsForCsounds[csound].clear();
        }
    }
    csound->UnlockMutex(cs_sfg_ports);
//#pragma omp critical (cs_sfg_ftables)
    csound->LockMutex(cs_sfg_ftables);
    {
        if (functionTablesForCsoundsForEvtblks.find(csound) != functionTablesForCsoundsForEvtblks.end()) {
            functionTablesForCsoundsForEvtblks[csound].clear();
        }
    }
    csound->UnlockMutex(cs_sfg_ftables);
    return 0;
  }
}

}
