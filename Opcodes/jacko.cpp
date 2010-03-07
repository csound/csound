/**
 * T H E   J A C K   O P C O D E S
 * Michael Gogins
 *
 * The Jack opcodes can be used to connect any number
 * of Csound instances, instruments, or user-defined
 * opcodes to any number of Jack ports
 * in any number of external Jack servers. 
 *
 * Audio and MIDI signal types are supported.
 *
 * The major purpose of the opcodes is to simplify
 * the use of external Jack-enabled synthesizers 
 * within Csound, and also to simplify the use of 
 * Csound within Jack-enabled hosts.
 *
 * O P C O D E S
 *
 * jackinit Sclientname, Servername
 *
 * Initializes the Jack opcodes; must be called once in the
 * orchestra header. Normally, the client name should be 
 * "csound" and the server name should be "default."
 *
 * jackinfo 
 * 
 * Prints all Jack port names, types, and states at 
 * the beginning of performance; may be called in the
 * orchestra header, after jackinit. 
 *
 * jackfreewheel isfree
 *
 * Turns freewheeling mode on or off. When freewheeling
 * mode is on, if supported by the rest of the Jack system,
 * Csound will run as fast as possible, which may be either
 * faster or slower than real time.
 * 
 * asignal jackaudioin Sexternalportname, Smyportname
 *
 * Receives an audio signal from the named output port
 * (Csound creates a input port with an internal name).
 *
 * jackaudioout Smyportname, Sexternalportname, asignal
 *
 * Sends an audio signal to the named input port
 * (Csound creates a output port with an internal name).
 * Audio from multiple instances of the opcode sending
 * to the same port is summed before sending.
 *
 * kpending jackmidiinpending Smyportname
 * 
 * Returns true if any MIDI events are pending in the port.
 * This can be used to control a loop for reading any number
 * of events in the same kperiod.
 *
 * kstatus, kchannel, kdata1, kdata2 jackmidiin Sportname
 *
 * Receives one MIDI channel message from the port.
 *
 * jackmidiout Smyportname, Sexternalportname, kstatus, kchannel, kdata1[, kdata2]
 *
 * Sends one MIDI channel message to the port. This can be called
 * any number of times in the same kperiod. Messages from multiple
 * instances of the opcode sending to the same port are collected
 * before sending.
 *
 * jacknoteout Smyportname, Sexternalportname, ichannel, ikey, ivelocity
 *
 * Sends one note to the port. The appropriate MIDI note off 
 * events is generated and scheduled for later transmission. 
 * Notes from multiple instances of the opcode sending to 
 * the same port are collected before sending.
 *
 * I M P L E M E N T A T I O N
 *
 * Assumptions:
 *
 * 1. Csound's sr must be equal to Jack's frames per second.
 * 2. Csound's ksmps must be equal to Jack's frames per period.
 * 3. It is possible to use the wait/signal processing model
 *    together with the existing Csound thread, instead of creating
 *    a thread for Jack. If this is not the case, the callback 
 *    model should be used.
 *
 * The processing algorithm uses Jack's wait/signal processing
 * model, not the callback model:
 *
 * 1. The jackinit opcode creates a client;
 *    the client is associated with the instance of Csound, 
 *    so that the code is re-entrant.
 * 2. The opcode init functions create and activate an internal port, 
 *    if one has not already been created for that opcode;
 *    each port is associated with its opcode;
 *    the internal port is then connected with the indicated 
 *    external port.
 * 3. The very first opcode to run, the very first time it runs,
 *    activates the client before doing anything else.
 * 4. In each kperiod, the very last opcode to run, 
 *    after finishing everything else,
 *    first signals Jack, then waits for Jack.
 * 5. The module deinitialization function deactivates the client 
 *    and destroys all state.
 *
 * Signals that pass from Jack output ports,
 * through Csound processing, and to Jack input ports,
 * must be properly ordered by order of instrument and opcode declaration
 * within Csound. 
 *
 * Processing with the callback model:
 *
 * 1. The jackinit opcode creates a Jack client, 
 *    which is associated with the running instance of Csound;
 *    registers a JackProcessCallback  with Jack; 
 *    registers a SenseEventCallback with Csound.
 * 2. The SenseEventCallback activates the client, 
 *    if it has not yet been activated; signals 
 *    the Jack processing callback, and waits
 *    while Jack computes the rest of its processing cycle.
 * 3. The Jack processing callback signals 
 *    the SenseEventsCallback, and waits
 *    while Csound computes the rest of its kperiod.
 * 4. Csound then performs one kperiod, which involves
 *    copying data from Jack output port buffers into any number
 *    of Jack opcode input buffers, and summing data from
 *    any number of Jack opcode outbut buffers to Jack
 *    input port buffers.
 * 5. At the end of processing, the module deinitialization
 *    function closes the client and erases all state.
 */
#include <OpcodeBase.hpp>
#include <cstdlib>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <map>
#include <pthread.h>
#include <set>
#include <string>
#include <vector>

struct JackInit;
struct JackInfo;
struct JackFreewheel;
struct JackAudioIn;
struct JackAudioOut;
struct JackMidiOut;
struct JackNoteOut;
struct JackState;

int JackProcessCallback_(jack_nframes_t frames, void *data);

void SenseEventCallback_(CSOUND * csound, void *data);

static std::map<CSOUND *, JackState *> jackStatesForCsoundInstances;

/**
 * Manages all state relevant to the global
 * interaction between Jack and Csound for a particular
 * client and instance of Csound.
 * This amounts basically to synchronizing when
 * Jack and Csound are processing data. All other
 * Jack state and operations, including the creation of
 * Jack ports and connections, is managed by the 
 * individual opcodes.
 */
struct JackState
{
  CSOUND *csound;
  jack_client_t *client;
  void *jacklock;
  void *csoundlock;
  bool activated;
  size_t waitMilliseconds;
  jack_nframes_t ksmps;
  jack_nframes_t jackKsmps;
  jack_nframes_t sr;
  jack_nframes_t jackSr;
  std::map<jack_port_t *, std::vector<void *> > opcodesForMidiOutPorts;
  JackState(CSOUND *csound_, const char *servername, const char *clientname) : 
    csound(csound_), 
    activated(false),
    waitMilliseconds(500)
  {
    csound = csound_;
    ksmps = csound->GetKsmps(csound);
    sr = csound->GetSr(csound);
    jack_options_t jack_options = (jack_options_t) JackOpenOptions;
    jack_status_t status = jack_status_t(0);
    client = jack_client_open(clientname,
			      jack_options, 
			      &status,
			      servername);
    if (!client) {
      csound->Message(csound, "Could not create Jack client \"%s\" -- is Jack server \"%s\" running? Status: %d\n", 
		      clientname, 
		      servername,
		      status);
      csound->LongJmp(csound, 1);
    } else {
      csound->Message(csound, "Created Jack client \"%s\" for Jack server \"%s\".\n", clientname, servername);
    }
    jackKsmps = jack_get_buffer_size(client);
    if (ksmps != jackKsmps) {
      csound->Message(csound, 
		      "Jack buffer size %d != Csound ksmps %d, exiting...\n",
		      jackKsmps, 
		      ksmps);
      csound->LongJmp(csound, 1);
    }
    jackSr = jack_get_sample_rate(client);
    if (sr != jackSr) {
      csound->Message(csound, 
		      "Jack sampling rate %d != Csound sr %d, exiting...\n",
		      jackSr, 
		      sr);
      csound->LongJmp(csound, 1);
    }
    jackStatesForCsoundInstances[csound] = this;
    jacklock = csound->CreateThreadLock();
    csoundlock = csound->CreateThreadLock();
    csound->RegisterSenseEventCallback(csound, SenseEventCallback_, this);
    int result = jack_set_process_callback(client, JackProcessCallback_, this);
    activated = false;
    result = jack_activate(client);
    if (!result) {
      csound->Message(csound, 
		      "Activated Jack client \"%s\".\n", 
		      jack_get_client_name(client));
    } else {
      csound->Message(csound, 
		      "Failed to activate Jack client \"%s\": status %d.\n", 
		      jack_get_client_name(client), 
		      result);
      activated = false;
    }
  }
  ~JackState()
  {
    csound->Message(csound, "BEGAN JackState::~JackState()...\n");
    int result = jack_client_close(client);
    csound->NotifyThreadLock(csoundlock);
    csound->NotifyThreadLock(jacklock);
    csound->DestroyThreadLock(csoundlock);
    csound->DestroyThreadLock(jacklock);
    jackStatesForCsoundInstances.erase(csound); 
    csound->Message(csound, "ENDED JackState::~JackState().\n");
    
  }
  int processJack(jack_nframes_t frames)
  {
    if (!activated) {
      activated = true;
    } else {
      csound->NotifyThreadLock(csoundlock);
      int result = csound->WaitThreadLock(jacklock, waitMilliseconds);
      if (result) {
	csound->Message(csound, "Timed out in JackState::processJack: %d\n", result);
      }
    }
    return 0;
  }
  int processCsound()
  {
    if (activated) {
      csound->NotifyThreadLock(jacklock);
      int result = csound->WaitThreadLock(csoundlock, waitMilliseconds);
      if (result) {
	csound->Message(csound, "Timed out in JackState::processCsound: %d\n", result);
      }
    }
    return 0;
  }
};

static JackState *getJackState(CSOUND *csound)
{
  return jackStatesForCsoundInstances[csound];
}

int JackProcessCallback_(jack_nframes_t frames, void *data)
{
  return ((JackState *)data)->processJack(frames);
}

void SenseEventCallback_(CSOUND * csound, void *data)
{
  ((JackState *)data)->processCsound();
}

struct JackInit : public OpcodeBase<JackInit>
{
  MYFLT *Servername;
  MYFLT *Sclientname;
  const char *servername;
  const char *clientname;
  JackState *jackState;
  jack_status_t status;
  int init(CSOUND *csound)
  {
    servername = csound->strarg2name(csound,
				     (char *) 0,
				     Servername,
				     (char *)"default",
				     (int) csound->GetInputArgSMask(this));
    clientname = csound->strarg2name(csound,
				     (char *) 0,
				     Sclientname,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jackState = new JackState(csound, servername, clientname);
    return OK;
  }
};

struct JackInfo : public OpcodeBase<JackInfo>
{
  JackState *jackState;
  int init(CSOUND *csound)
  {
    jackState = getJackState(csound);
    const char **ports = jack_get_ports(jackState->client, 0, 0, 0);
    if (ports) {
      log(csound, "Current Jack ports and their connections:\n");
      for (size_t i = 0; ports[i]; ++i) {
	const char *portname = ports[i];
	jack_port_t *port = jack_port_by_name(jackState->client, portname);
	int flags = jack_port_flags(port);
	const char *type = jack_port_type(port);
	log(csound, "%3d:  Flags: %3d  Type: %-25s  Name: %s\n", (i+1), flags, type, portname);
	const char **connections = jack_port_get_all_connections(jackState->client, port);
	if (connections) {
	  for (size_t j = 0; connections[j]; ++j) {
	    log(csound, "      Connected to:                                      --> %s\n", connections[j]);
	  }
	}
	std::free(connections);
      }
      std::free(ports);
    }
    return OK;
  }
};

struct JackFreewheel : public OpcodeBase<JackFreewheel>
{
  MYFLT *ifreewheel;
  JackState *jackState;
  int init(CSOUND *csound)
  {
    jackState = getJackState(csound);
    int freewheel = (int) *ifreewheel;
    int result = jack_set_freewheel(jackState->client, freewheel);
    if (result) {
      warn(csound, "Failed to set Jack freewheeling mode to \"%s\": error %d.\n", (freewheel ? "on" : "off"), result);
    } else {
      log(csound, "Set Jack freewheeling mode to \"%s\".\n", (freewheel ? "on" : "off"));
    }
    return result;
  }
};

struct JackAudioIn : public OpcodeBase<JackAudioIn>
{
  // Out.
  MYFLT *asignal;
  // Ins.
  MYFLT *Sotherportname;
  MYFLT *Smyportname;
  // State.
  const char *myportname;
  char myfullportname[0x100];
  const char *otherportname;
  const char *clientname;
  size_t frames;
  JackState *jackState;
  jack_port_t *myport;
  jack_port_t *otherport;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientname = jack_get_client_name(jackState->client);
    myportname = csound->strarg2name(csound,
				   (char *)0,
				   Smyportname,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(myfullportname, "%s:%s", clientname, myportname);
    otherportname = csound->strarg2name(csound,
				     (char *) 0,
				     Sotherportname,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->client, myfullportname);
    if (!port_) {
      myport = jack_port_register(jackState->client, myportname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
      if (myport) {
	log(csound, "Created port \"%s\".\n", myfullportname);
      } else {
	warn(csound, "Could not create port \"%s\".\n", myfullportname);
      }
      otherport = jack_port_by_name(jackState->client, otherportname);
      result = jack_connect(jackState->client, jack_port_name(otherport), jack_port_name(myport));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    otherportname, 
	    myfullportname);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     otherportname, 
	     myfullportname,
	     result);
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    otherportname, 
	    myfullportname);
      }
    }
    return result;
  }
  int audio(CSOUND *csound)
  {
    jack_default_audio_sample_t *buffer = (jack_default_audio_sample_t *)jack_port_get_buffer(myport, frames);
    for (size_t frame = 0; frame < frames; ++frame) {
      asignal[frame] = buffer[frame];
    }
    return OK;
  }
};

struct JackAudioOut : public OpcodeBase<JackAudioOut>
{
  // No outs.
  // Ins.
  MYFLT *Smyportname;
  MYFLT *Sotherportname;
  MYFLT *asignal;
  // State.
  const char *myportname;
  char myfullportname[0x100];
  const char *otherportname;
  const char *clientname;
  size_t frames;
  JackState *jackState;
  jack_port_t *myport;
  jack_port_t *otherport;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientname = jack_get_client_name(jackState->client);
    myportname = csound->strarg2name(csound,
				   (char *)0,
				   Smyportname,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(myfullportname, "%s:%s", clientname, myportname);
    otherportname = csound->strarg2name(csound,
				     (char *) 0,
				     Sotherportname,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->client, myfullportname);
    if (!port_) {
      myport = jack_port_register(jackState->client, myportname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
      if (myport) {
	log(csound, "Created port \"%s\".\n", myfullportname);
      } else {
	warn(csound, "Could not create port \"%s\".\n", myfullportname);
      }
      otherport = jack_port_by_name(jackState->client, otherportname);
      result = jack_connect(jackState->client, jack_port_name(myport), jack_port_name(otherport));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    myfullportname, 
	    otherportname);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     myfullportname, 
	     otherportname, 
	     result);
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    myfullportname, 
	    otherportname);
      }
    }
    return result;
  }
  int audio(CSOUND *csound)
  {
    jack_default_audio_sample_t *buffer = (jack_default_audio_sample_t *)jack_port_get_buffer(myport, frames);
    for (size_t frame = 0; frame < frames; ++frame) {
      buffer[frame] += asignal[frame];
    }
    return OK;
  }
};

struct JackMidiOut : public OpcodeBase<JackMidiOut>
{
  // No outs.
  // Ins.
  MYFLT *Smyportname;
  MYFLT *Sotherportname;
  MYFLT *kstatus;
  MYFLT *kchannel;
  MYFLT *kdata1;
  MYFLT *kdata2;
  char status;
  char channel;
  char data1;
  char data2;
  char priorstatus;
  char priorchannel;
  char priordata1;
  char priordata2;
  // State.
  const char *myportname;
  char myfullportname[0x100] ;
  const char *otherportname;
  const char *clientname;
  jack_port_t *myport;
  jack_port_t *otherport;
  size_t frames;
  JackState *jackState;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientname = jack_get_client_name(jackState->client);
    myportname = csound->strarg2name(csound,
				   (char *)0,
				   Smyportname,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(myfullportname, "%s:%s", clientname, myportname);
    otherportname = csound->strarg2name(csound,
				     (char *) 0,
				     Sotherportname,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->client, myfullportname);
    if (!port_) {
      myport = jack_port_register(jackState->client, myportname, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
      if (myport) {
	log(csound, "Created port \"%s\".\n", myfullportname);
      } else {
	warn(csound, "Could not create port \"%s\".\n", myfullportname);
      }
      otherport = jack_port_by_name(jackState->client, otherportname);
      result = jack_connect(jackState->client, jack_port_name(myport), jack_port_name(otherport));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    myfullportname, 
	    otherportname);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     myfullportname, 
	     otherportname, 
	     result);
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    myfullportname, 
	    otherportname);
      }
      jackState->opcodesForMidiOutPorts[myport].push_back(this);
    }
    priorstatus = -1;
    priorchannel = -1;
    priordata1 = -1;
    priordata2 = -1;
    return result;
  }
  int kontrol(CSOUND *csound)
  {
    int result = OK;
    jack_nframes_t deltaframes = 0;
    status = *kstatus;
    channel = *kchannel;
    data1 = *kdata1;
    data2 = *kdata2;
    buffer = (jack_midi_data_t *)jack_port_get_buffer(myport, frames);
    if (this == jackState->opcodesForMidiOutPorts[myport].front()) {
      jack_midi_clear_buffer(buffer);
    }
    if (status != priorstatus ||
	channel != priorchannel ||
	data1 != priordata1 ||
	data2 != priordata2) {
      size_t dataSize = 0;
      if (data2 == -1) {
	dataSize = 2;
      } else {
	dataSize = 3;
      }
      jack_midi_data_t *data = jack_midi_event_reserve(buffer, deltaframes, dataSize);
      data[0] = (status || channel);
      data[1] = data1;
      if (data2 != -1) {
	data[2] = data2;
      }
    }
    priorstatus = status;
    priorchannel = channel;
    priordata1 = data1;
    priordata2 = data2;
    return result;
  }
};

struct JackNoteOut : public OpcodeNoteoffBase<JackNoteOut>
{
  // No outs.
  // Ins.
  MYFLT *Smyportname;
  MYFLT *Sotherportname;
  MYFLT *ichannel;
  MYFLT *ikey;
  MYFLT *ivelocity;
  char status;
  char channel;
  char key;
  char velocity;
  // State.
  const char *myportname;
  char myfullportname[0x100] ;
  const char *otherportname;
  const char *clientname;
  jack_port_t *myport;
  jack_port_t *otherport;
  size_t frames;
  JackState *jackState;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientname = jack_get_client_name(jackState->client);
    myportname = csound->strarg2name(csound,
				   (char *)0,
				   Smyportname,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(myfullportname, "%s:%s", clientname, myportname);
    otherportname = csound->strarg2name(csound,
				     (char *) 0,
				     Sotherportname,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->client, myfullportname);
    if (!port_) {
      myport = jack_port_register(jackState->client, myportname, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
      if (myport) {
	log(csound, "Created port \"%s\".\n", myfullportname);
      } else {
	warn(csound, "Could not create port \"%s\".\n", myfullportname);
      }
      otherport = jack_port_by_name(jackState->client, otherportname);
      result = jack_connect(jackState->client, jack_port_name(myport), jack_port_name(otherport));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    myfullportname, 
	    otherportname);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     myfullportname, 
	     otherportname, 
	     result);
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    myfullportname, 
	    otherportname);
      }
      jackState->opcodesForMidiOutPorts[myport].push_back(this);
    }
    jack_nframes_t deltaframes = 0;
    status = 144;
    channel = (char) *ichannel;
    key = (char) *ikey;
    velocity = (char) *ivelocity;
    buffer = (jack_midi_data_t *)jack_port_get_buffer(myport, frames);
    if (this == jackState->opcodesForMidiOutPorts[myport].front()) {
      jack_midi_clear_buffer(buffer);
    }
    jack_midi_data_t *data = jack_midi_event_reserve(buffer, deltaframes, 3);
    data[0] = (status || channel);
    data[1] = key;
    data[2] = velocity;  
    return result;
  }
  int noteoff(CSOUND *csound)
  {
    int result = OK;
    jack_nframes_t deltaframes = 0;
    buffer = (jack_midi_data_t *)jack_port_get_buffer(myport, frames);
    if (this == jackState->opcodesForMidiOutPorts[myport].front()) {
      jack_midi_clear_buffer(buffer);
    }
    jack_midi_data_t *data = jack_midi_event_reserve(buffer, deltaframes, 3);
    data[0] = (status || channel);
    data[1] = key;
    data[2] = 0;  
    return result;
  }
};

extern "C"
{
  static OENTRY oentries[] = {
    {
      (char *)"jackinit",
      sizeof(JackInit),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackInit::init_,
      0,
      0
    },
    {
      (char *)"jackinfo",
      sizeof(JackInfo),
      1,
      (char *)"",
      (char *)"",
      (SUBR)&JackInfo::init_,
      0,
      0
    },
    {
      (char *)"jackfreewheel",
      sizeof(JackFreewheel),
      1,
      (char *)"",
      (char *)"i",
      (SUBR)&JackFreewheel::init_,
      0,
      0
    },
    {
      (char *)"jackaudioin",
      sizeof(JackAudioIn),
      5,
      (char *)"a",
      (char *)"SS",
      (SUBR)&JackAudioIn::init_,
      0,
      (SUBR)&JackAudioIn::audio_,
    },
    {
      (char *)"jackaudioout",
      sizeof(JackAudioOut),
      5,
      (char *)"",
      (char *)"SSa",
      (SUBR)&JackAudioOut::init_,
      0,
      (SUBR)&JackAudioOut::audio_,
    },
    {
      (char *)"jackmidiout",
      sizeof(JackMidiOut),
      3,
      (char *)"",
      (char *)"SSkkkj",
      (SUBR)&JackMidiOut::init_,
      (SUBR)&JackMidiOut::kontrol_,
      0,
    },
    {
      (char *)"jacknoteout",
      sizeof(JackNoteOut),
      3,
      (char *)"",
      (char *)"SSiii",
      (SUBR)&JackNoteOut::init_,
      (SUBR)&JackNoteOut::kontrol_,
      0
    },
    { 0, 0, 0, 0, 0, (SUBR) 0, (SUBR) 0, (SUBR) 0 }
 };


  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }
  
  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    OENTRY *ep = (OENTRY *)&(oentries[0]);
    int  err = 0;
    while (ep->opname != 0) {
      err |= csound->AppendOpcode(csound,
				  ep->opname, 
				  ep->dsblksiz, 
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
    int result = OK;
#pragma omp critical
    {
      //csound->Message(csound, "jacko: CsoundModuleDestroy(%p)\n", csound);
      delete jackStatesForCsoundInstances[csound];
    }
    return result;
  }
}


