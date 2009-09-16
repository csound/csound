#include "OpcodeBase.hpp"
#include <map>
#include <string>
#include <vector>
/**
 * T H E   S I G N A L   F L O W   G R A P H   
 * F A M I L Y   O F   O P C O D E S
 *
 * Michael Gogins
 *
 * These opcodes enable the declaration 
 * of signal flow graphs (AKA asynchronous data flow graphs)
 * in Csound orchestras. Signals flow 
 * from the outlets of source instruments
 * to the inlets of sink instruments.
 * Signals may be k-rate, a-rate, or f-rate.
 * Any number of outlets may be connected 
 * to any number of inlets. When a new instance 
 * of an instrument is instantiated 
 * during performance, the declared connections also 
 * are automatically instantiated.
 *
 * Signal flow graphs simplify the construction of complex mixers,
 * signal processing chains, and the like. They also simplify the
 * re-use of "plug and play" instrument definitions
 * and even entire sub-orchestras, 
 * which can simply be #included and then "plugged in" 
 * to existing orchestras.
 *
 * Instruments must be named, and each source instrument 
 * must be defined in the orchestra before any of its sinks.
 * The reason instruments must be named is so that 
 * outlets and inlets in any higher-level orchestra can
 * be connected to inlets and outlets in any lower-level 
 * #included orchestra. 
 *
 * O P C O D E S
 * 
 * signalflowgraph
 *
 * Initializes the signal flow graph; must be declared once 
 * and only once in the top-level orchestra,
 * before any of the other signal flow graph opcodes.
 *
 * outleta Sname, asignal
 * outletk Sname, ksignal
 * outletf Sname, fsignal
 *
 * Outlets send a, k, or f-rate signals out from an instrument.
 *
 * The name of the outlet is implicitly qualified by the instrument name,
 * so it is valid to use the same outlet name in more than one instrument
 * (but not to use the same outlet name twice in the same instrument).
 *
 * asignal inleta Sname
 * ksignal inletk Sname
 * fsignal inletf Sname
 *
 * Inlets receive a, k, or f-rate signals from outlets in other instruments.
 * The signals from all the source outlet instances are summed 
 * in each sink inlet instance.
 *
 * The name of the inlet is implicitly qualified by the instrument name,
 * so it is valid to use the same inlet name in more than one instrument
 * (but not to use the same inlet name twice in the same instrument).
 *
 * connect Source1, Soutlet1, Sink1, Sinlet1 [[, Source2, Soutlet2, Sink1, Sinlet2] ...]]
 *
 * The connect opcode, valid only in orchestra headers, sends the signals
 * from the indicated outlets in all instances of the indicated source
 * instrument to the indicated inlets in all instances of the indicated sink
 * instrument. 
 * 
 * alwayson Sinstrumentname [p4, ..., pn]
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
 * The optional pfields are sent to the instrument following p3.
 *
 * ifno ftgenonce isize, igen [, iarga, iargb, ...]
 *
 * Enables the creation of function tables 
 * entirely inside instrument definitions,
 * without any duplication of data.
 *
 * The ftgenonce opcode is designed to simplify 
 * writing instrument definitions that can be 
 * re-used in different orchestras simply by #including them
 * and plugging them into some output instrument. 
 * There is no need to define function tables 
 * either in the score, or in the orchestra header.
 *
 * The ftgenonce opcode is similar to ftgentmp, 
 * except that function tables are neither 
 * duplicated nor deleted. Instead, all of the arguments
 * to the opcode are concatenated to form the key 
 * to a lookup table that points to the function
 * table structure. Every request to ftgenonce 
 * with the same arguments receives the same
 * instance of the function table data. 
 * Every change in the value of any ftgenonce argument
 * causes the creation of a new function table.
 */

struct SignalFlowGraph;
struct Outleta;
struct Outletk;
struct Outletf;
struct Inleta;
struct Inletk;
struct Inletf;
struct Connect;
struct AlwaysOn;
struct FtGenOnce;

// Internally, identifiers are always "sourcename:outletname" or "sinkname:inletname".

std::map<CSOUND * /* instance */, std::map< std::string /* source_outlet */, std::vector< Outleta * /* outlets */ > > > \
aoutletsForInstancesForSourcesForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* source_outlet */, std::vector< Outletk * /* outlets */ > > > \
koutletsForInstancesForSourcesForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* source_outlet */, std::vector< Outletf * /* outlets */ > > > \
kfoutletsForInstancesForSourcesForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* sink_inlet */, std::vector< Inleta * /* inlets */ > > > \
ainletsForInstancesForSinksForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* sink_inlet */, std::vector< Inletk * /* inlets */ > > > \
kinletsForInstancesForSinksForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* sink_inlet */, std::vector< Inletf * /* inlets */ > > > \
finletsForInstancesForSinksForNames;
std::map<CSOUND * /* instance */, std::map< std::string /* sink_inlet */, std::vector< std::string /* source_outlets */ > > > connections;

/**
 * All it does is clear the data structures, 
 * in case they are full from a previous performance.
 */
struct SignalFlowGraph : public OpcodeBase<SignalFlowGraph
{
  int init(CSOUND *csound)
  {
    aoutletsForInstancesForSourcesForNames[csound].clear();
    koutletsForInstancesForSourcesForNames[csound].clear();
    foutletsForInstancesForSourcesForNames[csound].clear();
    ainletsForInstancesForSinksForNames[csound].clear();
    kinletsForInstancesForSinksforNames[csound].clear();
    finletsForInstancesForSinksForNames[csound].clear();
    connections[csound].clear();
    return OK;
  };
};

struct Outleta : public OpcodeBase<Outleta>
{
  /**
   * Output.
   */
  MYFLT *asignal;
  /**
   * Input.
   */
  MYFLT *Sname;
  /**
   * State.
   */
  std::string sourceIdentifier;
  int init(CSOUND *csound)
  {
    // May need to convert insno to name if not named.
    std::string source = ((INSTRTXT *)h.insdshead->optxt)->instrname;
    std::string name = csound->strarg2name(csound,
					   (char*) NULL,
					   Sname,
					   (char *)"",
					   (int) csound->GetInputArgSMask(this));
    sourceIdentifier = source + ":" name;
    std::vector<Outleta *> &aoutlets = aoutletsForInstancesForSourcesForNames[csound][sourceIdentifier];
    if (std::find(aoutlets.begin(), aoutlets.end(), this) == aoutlets.end()) {
      aoutlets.push_back(this);
    }
    return OK;
  }
};

struct Inleta : public OpcodeBase<Inleta>
{
  /**
   * Inputs.
   */
  MYFLT *Sname;
  MYFLT *asignal;
  /**
   * State.
   */
  std::string sinkIdentifier;
  std::vector< std::vector<Outleta *> *> sourceOutlets;
  int init(CSOUND *csound)
  {
    // May need to convert insno to name if not named.
    std::string sink = ((INSTRTXT *)h.insdshead->optxt)->instrname;
    std::string name = csound->strarg2name(csound,
					   (char*) NULL,
					   Sname,
					   (char *)"",
					   (int) csound->GetInputArgSMask(this));
    sinkIdentifier = sink + ":" + name;
    std::vector<Inleta *> &ainlets = ainletsForInstancesForSinksForNames[csound][identifier];
    if (std::find(ainlets.begin(), ainlets.end(), this) == ainlets.end()) {
      ainlets.push_back(this);
    }
    // Find source outlets connecting to this.
    // Any number of sources may connect to any number of sinks.
    std::vector<std::string> &sourceIdentifiers = connections[csound][sinkIdentifier];
    for (size_t i = 0, n = sourceIdentifiers.size(); i < n; i++) {
      std::string sourceIdentifier = sourceIdentifiers[i];
      std::vector<Outleta *> *aoutlets = &aoutletsForInstancesForSourcesForNames[csound][sourceIdentifier];
      if (std::find(aoutlets, sourceOutlets.begin(), sourceOutlets.end()) != sourceOutlets.end()) {
	sourceOutlets.push_back(aoutlets);
      }
      return OK;
    }
  };

  struct Connect : public OpcodeBase<Connect>
  {
    MYFLT *Source;
    MYFLT *Soutlet;
    MYFLT *Sink;
    MYFLT *Sinlet;
    int init(CSOUND *csound)
    {
      std::string source = csound->strarg2name(csound,
					       (char*) NULL,
					       Source,
					       (char *)"",
					       (int) csound->GetInputArgSMask(this));
      std::string outlet = csound->strarg2name(csound,
					       (char*) NULL,
					       Soutlet,
					       (char *)"",
					       (int) csound->GetInputArgSMask(this));
      std::string source_outlet = source + ":" + outlet;
      std::string sink = csound->strarg2name(csound,
					     (char*) NULL,
					     Sink,
					     (char *)"",
					     (int) csound->GetInputArgSMask(this));
      std::string inlet = csound->strarg2name(csound,
					      (char*) NULL,
					      Sinlet,
					      (char *)"",
					      (int) csound->GetInputArgSMask(this));
      std::string sink_inlet = sink + ":" + inlet;
      connections[csound][sinkIdentifer].push_back(sourceIdentifier);
      return OK;
    }
  };

  struct AlwaysOn : public OpcodeBase<AlwaysOn>
  {
    MYFLT *Sinstrument;
    MYFLT *argums[VARGMAX];
    int init(CSOUND *csound)
    {
      return OK;
    }
 };

  struct FtGenOnce : public OpcodeBase<FtGenOnce>
  {
    MYFLT *Sinstrument;
    MYFLT *argums[VARGMAX];
    int init(CSOUND *csound)
    {
      return OK;
    }
 };

  extern "C"
  {

    static OENTRY localops[] = {
      {
	(char*)"signalflowgraph",
	sizeof(SignalFlowGraph),
	1,
	(char*)"",
	(char*)"",
	(SUBR)&SignalFlowGraph::init_,
	0,
	0,
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

