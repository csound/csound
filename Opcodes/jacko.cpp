/**
 * T H E   J A C K   O P C O D E S
 * Michael Gogins
 *
 * The Jack opcodes can be used to connect any number
 * of Csound instances, instruments, or user-defined
 * opcodes to any number of Jack ports
 * in any number of external Jack servers. 
 * Audio and MIDI signal types are supported.
 *
 * Sending MIDI and/or audio to external Jack clients,
 * such as synthesizers, and receiving audio and/or 
 * MIDI back from them, is supported.
 *
 * Receiving MIDI and/or audio from external Jack clients,
 * such as sequencers, and sending MIDI and/or audio 
 * back to them, is supported.
 *
 * Other uses also are supported.
 *
 * O P C O D E S
 *
 *
 * JackInit -- Initializes Csound as a Jack client.
 *
 * Description
 *
 * Initializes this instance of Csound as a Jack client.
 *
 * Csound's sr must be equal to the Jack daemon's 
 * frames per second.
 * 
 * Csound's ksmps must be equal to the Jack daemon's 
 * frames per period.
 *
 * Frames per period must not only (a) be a power of 2,
 * but also (b) go evenly into the frames per second,
 * e.g. 128 frames per period goes into 48000 
 * frames per second 375 times, for a latency or 
 * MIDI time granularity of about 2.7 milliseconds
 * (as good as or better than the absolute best 
 * human performers). 
 *
 * The order of processing of all signals that pass 
 * from Jack input ports, through Csound processing, 
 * and to Jack output ports, must be properly
 * determined by sequence of instrument and 
 * opcode definition within Csound.
 *
 * Syntax
 *
 * JackInit SclientName, ServerName
 *
 * Initialization
 *
 * SclientName -- The name of the Jack client; 
 * normally, should be "csound". 
 *
 * ServerName -- The name of the Jack daemon; 
 * normally, will be "default".
 * 
 * This opcode must be called once and only once in the 
 * orchestra header, and before any other Jack opcodes. 
 *
 *
 * JackInfo -- Prints information about the Jack system.
 *
 * Description
 *
 * Prints the Jack daemon and client names, the
 * sampling rate and frames per period, 
 * and all active Jack port names, 
 * types, states, and connections.
 *
 * Syntax
 *
 * JackInfo
 *
 * Initialization
 *
 * May be called any number of times in the orchestra header,
 * for example both before and after creating Jack ports
 * in the Csound orchestra header.
 *
 *
 * JackFreewheel -- Turns freewheeling mode on or off.
 *
 * Description
 *
 * Turns Jack's freewheeling mode on or off. 
 *
 * When freewheeling is on, if supported by the rest 
 * of the Jack system, Csound will run as fast as possible, 
 * which may be either faster or slower than real time.
 *
 * This is essential for rendering scores that are too
 * dense for real-time performance to a soundfile, 
 * without xruns or dropouts.
 *
 * Syntax  
 *
 * JackFreewheel [ienabled]
 *
 * Initialization
 *
 * ienabled -- Turns freewheeling on (the default) or off. 
 *
 *
 * JackAudioInConnect -- Creates an audio connection
 *                       from a Jack port to Csound.
 *
 * Description
 *
 * Creates an audio connection from an external Jack 
 * audio output port to a Jack audio input port inside 
 * this instance of Csound.
 *
 * Syntax
 *
 * JackAudioInConnect SexternalPortName, ScsoundPortName
 *
 * Initialization
 *
 * SexternalPortName -- The full name ("clientname:portname") 
 * of an external Jack audio output port.
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack audio input port.
 *
 * Performance
 *
 * The actual audio must be read with the JackAudioIn opcode.
 *
 *
 * JackAudioOutConnect-- Creates an audio connection 
 *                        from Csound to a Jack port.
 *
 * Description
 *
 * In the orchestra header, creates an audio connection 
 * from a Jack audio output port inside this instance 
 * of Csound to an external Jack audio input port.
 *
 * Syntax
 * 
 * JackAudioOutConnect ScsoundPortName, SexternalPortName
 *
 * Initialization
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack audio input port.
 *
 * SexternalPortName -- The full name ("clientname:portname") 
 * of an external Jack audio output port.
 *
 * Performance
 * 
 * The actual audio must be written with the JackAudioOut 
 * opcode.
 *
 *
 * JackMidiInConnect -- Creates a MIDI connection from
 *                      Jack to Csound.
 *
 * Description
 *
 * Creates a MIDI connection from an external Jack MIDI 
 * output port to this instance of Csound.
 *
 * Syntax
 *
 * JackMidiInConnect SexternalPortName, ScsoundPortName
 *
 * Initialization
 *
 * SexternalPortName -- The full name ("clientname:portname") 
 * of an external Jack MIDI output port.
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack MIDI input port.
 *
 * Must be used in conjunction with the 
 * -+rtmidi=null Csound command-line option. 
 * Can be used in with the MIDI inter-operability
 * command-line options and/or opcodes to enable the 
 * use of ordinary Csound instrument definitions to 
 * render external scores or MIDI sequences.
 *
 * Performance
 *
 * The actual  MIDI events will be received in the 
 * regular Csound way, i.e. through a MIDI driver 
 * and the sensevents mechanism, rather than through 
 * a Jack input port opcode. 
 *
 * The granularity of timing is Csound's kperiod.
 *
 *
 * JackMidiOutConnect -- Creates a MIDI connection from
 *                       csound to Jack.
 * 
 * Description
 *
 * In the orchestra header, creates a connection 
 * from a Jack MIDI output port inside this instance 
 * of Csound to an external Jack MIDI input port.
 *
 * Syntax
 *
 * JackMidiOutConnect ScsoundPortName, SexternalPortName
 *
 * Initialization
 * 
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack audio input port.
 *
 * SexternalPortName -- The full name ("clientname:portname") 
 * of an external Jack audio output port.
 *
 * Performance
 * 
 * The actual MIDI data must be written with the JackMidiOut 
 * or JackNoteOut opcodes.
 *
 *
 * JackOn -- Enables or disables all Jack opcodes.
 *
 * Description
 *
 * After all Jack connections have been created, enables
 * or disables all Jack input and output opcodes 
 * inside this instance of Csound to read or write data.
 *
 * Syntax
 *
 * JackOn [iactive]
 *
 * Initialization
 *
 * iactive -- A flag that turns the ports on (the default) 
 * or off.
 *
 * 
 * JackAudioIn -- Receives an audio signal from a Jack port.
 *
 * Description
 *
 * Receives an audio signal from a Jack audio input port 
 * inside this instance of Csound, which in turn has 
 * received the signal from its connected external Jack 
 * audio output port.
 *
 * Syntax
 *
 * asignal JackAudioIn ScsoundPortName
 *
 * Initialization
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack audio input port.
 *
 * Performance
 *
 * asignal -- Audio received from the external Jack
 * output port to which ScsoundPortName is connected.
 *
 *
 * JackAudioOut -- Sends an audio signal to a Jack port.
 *
 * Description
 * 
 * Sends an audio signal to an internal Jack audio 
 * output port, and in turn to its connected external 
 * Jack audio input port.
 *
 * Note that it is possible to send audio out via Jack
 * to the system audio interface, while at the same time 
 * rendering to a regular Csound output soundfile.
 *
 * Syntax
 *
 * JackAudioOut ScsoundPortName, asignal
 *
 * Initialization
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack audio output port.
 *
 * Performance
 *
 * asignal -- Audio to be sent to the external Jack audio 
 * input port to which CsoundPortName is connected.
 *
 * Audio from multiple instances of the opcode sending
 * to the same Jack port is summed before sending.
 *
 *
 * JackMidiOut -- Sends a MIDI channel message to a 
 *                Jack port.
 *
 * Description
 * 
 * Sends a MIDI channel message to a Jack MIDI output port
 * inside this instance of Csound, and in turn to its 
 * connected external Jack MIDI input port.
 *
 * Syntax
 *
 * JackMidiOut ScsoundPortName, kstatus, kchannel, kdata1[, kdata2]
 *
 * Initialization
 *
 * ScsoundPortName -- The short name ("portname") 
 * of the internal Jack MIDI input port.
 *
 * Performance
 *
 * kstatus -- MIDI status byte; must indicate a MIDI channel
 * message.
 *
 * kchannel -- MIDI channel (from 0 through 15).
 *
 * kdata1 -- First data byte of a MIDI channel message.
 *
 * kdata2 -- Optional second data byte of a MIDI channel message.
 *
 * This opcode can be called any number of times 
 * in the same kperiod. Messages from multiple instances 
 * of the opcode sending to the same port are collected 
 * before sending. 
 *
 * Running status, system exclusive messages, and 
 * real-time messages are not supported.
 * 
 * The granularity of timing is Csound's kperiod.
 *
 *
 * JackNoteOut -- Send one note to a Jack MIDI port.
 *
 * Description
 *
 * Sends one note to a Jack MIDI output port inside this 
 * instance of Csound, and in turn to the connected external
 * Jack MIDI input port, with a duration
 * specified by the score instrument statement. 
 * The matching MIDI note off message is generated 
 * and scheduled for later transmission. 
 *
 * Notes from multiple instances of the opcode 
 * sending to the same output port are collected 
 * before sending.
 *
 * The granularity of timing is Csound's kperiod.
 *
 * Syntax
 *
 * JackNoteOut ScsoundPortName, ichannel, ikey, ivelocity
 *
 * Initialization
 *
 * ScsoundPortName -- The short name ("portname") of the 
 * Jack MIDI output port created by jackmidioutconnect.
 *
 * ichannel -- The MIDI channel (from 0 through 15) to 
 * receive the note.
 *
 * ikey -- The MIDI key number (from 0 through 127, 60 is 
 * middle C) of the note.
 *
 * ivelocity -- The MIDI velocity number (from 0 through 127)
 * of the note.
 *
 *
 * I M P L E M E N T A T I O N
 *
 * Processing is done according to the callback model:
 *
 * 1. The jackinit opcode: 
 *    1.1. Creates a Jack client, 
 *         which is associated with the 
 *         running instance of Csound.
 *    1.2. Registers a JackProcessCallback with Jack. 
 *    1.3. Registers a SenseEventCallback with Csound.
 *    1.4. Installs a MIDI driver callback to consume
 *         MIDI events coming from Jack input ports.
 *    1.5. Puts the Csound processing thread to sleep.
 *    1.6. Activates the client.
 * 2. The ports are created and connected in the orchestra header.
 * 3. After all ports are connected, they are turned on in the 
 *    orchestra header.
 * 4. Every time the Jack callback fires:
 *    4.1. Any MIDI events pending in the input ports are enqueued
 *         for dispatch via the MIDI driver callack through 
 *         Csound's normal sensevents mechanism.
 *    4.2. csoundPerformKsmps is called.
 *    4.3. When the Csound performance is finished:
 *         4.3.1. The Csound processing thread is re-awakened.
 *         4.3.2. The Jack processing callback is deactivated.
 *         4.3.2. The Jack client is closed.
 * 5. At the end of processing, the module deinitialization
 *    function erases all Jack-related state.
 */
#include <csound.h>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <list>
#include <map>
#include <OpcodeBase.hpp>
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

static JackState *getJackState(CSOUND *csound);

static int JackProcessCallback_(jack_nframes_t frames, 
				void *data);

static void SenseEventCallback_(CSOUND *csound, 
				void *data);

static int midiDeviceOpen_(CSOUND *csound, 
			   void **userData,
			   const char *devName);

static int midiRead_(CSOUND *csound, 
		     void *userData,
		     unsigned char *midiData, 
		     int nbytes);

static std::map<CSOUND *, JackState *> jackStatesForCsoundInstances;

/**
 * Manages all state relevant to the global
 * interaction between Jack and Csound for a particular
 * client and instance of Csound. The actual reading
 * and writing of data is done by the input and output
 * opcodes.
 */
struct JackState
{
  CSOUND *csound;
  const char *serverName;
  const char *clientName;
  jack_client_t *jackClient;
  void *csoundThreadLock;
  char jackActive;
  char csoundActive;
  jack_nframes_t csoundFramesPerTick;
  jack_nframes_t jackFramesPerTick;
  jack_nframes_t csoundFramesPerSecond;
  jack_nframes_t jackFramesPerSecond;
  std::map<jack_port_t *, std::vector<void *> > opcodesForMidiOutPorts;
  std::map<std::string, jack_port_t *> audioInPorts;
  std::map<std::string, jack_port_t *> audioOutPorts;
  std::map<std::string, jack_port_t *> midiInPorts;
  std::map<std::string, jack_port_t *> midiOutPorts;
  std::list<unsigned char> midiInputQueue;
  JackState(CSOUND *csound_, const char *serverName_, const char *clientName_) : 
    csound(csound_), 
    serverName(serverName_),
    clientName(clientName_),
    jackActive(false),
    csoundActive(true)
  {
    csound = csound_;
    csoundFramesPerTick = csound->GetKsmps(csound);
    csoundFramesPerSecond = csound->GetSr(csound);
    jack_options_t jack_options = (jack_options_t) JackOpenOptions;
    jack_status_t status = jack_status_t(0);
    jackClient = jack_client_open(clientName,
			      jack_options, 
			      &status,
			      serverName);
    if (!jackClient) {
      csound->Message(csound, "Could not create Jack client \"%s\" -- is Jack server \"%s\" running? Status: %d\n", 
		      clientName, 
		      serverName,
		      status);
      csound->LongJmp(csound, 1);
    } else {
      csound->Message(csound, "Created Jack client \"%s\" for Jack server \"%s\".\n", clientName, serverName);
    }
    jackFramesPerTick = jack_get_buffer_size(jackClient);
    if (csoundFramesPerTick != jackFramesPerTick) {
      csound->Message(csound, 
		      "Jack buffer size %d != Csound ksmps %d, exiting...\n",
		      jackFramesPerTick, 
		      csoundFramesPerTick);
      csound->LongJmp(csound, 1);
    }
    jackFramesPerSecond = jack_get_sample_rate(jackClient);
    if (csoundFramesPerSecond != jackFramesPerSecond) {
      csound->Message(csound, 
		      "Jack sampling rate %d != Csound sr %d, exiting...\n",
		      jackFramesPerSecond, 
		      csoundFramesPerSecond);
      csound->LongJmp(csound, 1);
    }
    jackStatesForCsoundInstances[csound] = this;
    csoundThreadLock = csound->CreateThreadLock();
    csound->RegisterSenseEventCallback(csound, SenseEventCallback_, this);
    int result = jack_set_process_callback(jackClient, JackProcessCallback_, this);
    result = jack_activate(jackClient);
    if (!result) {
      csound->Message(csound, 
		      "Activated Jack client \"%s\".\n", 
		      jack_get_client_name(jackClient));
    } else {
      csound->Message(csound, 
		      "Failed to activate Jack client \"%s\": status %d.\n", 
		      jack_get_client_name(jackClient), 
		      result);
      return;
    }
    csound->SetExternalMidiInOpenCallback(csound, midiDeviceOpen_);
    csound->SetExternalMidiReadCallback(csound, midiRead_);
    csound->WaitThreadLockNoTimeout(csoundThreadLock);
  }
  ~JackState()
  {
    close();
    jackStatesForCsoundInstances.erase(csound); 
  }
  int close()
  {
    int result = OK;
    csound->Message(csound, "BEGAN JackState::close()...\n");
    // Try not to do thread related operations more than once...
    if (jackActive) {
      jackActive = false;
      result = jack_deactivate(jackClient);
      csound->Message(csound, "Deactivated Jack with result: %d.\n", result);
      result = jack_client_close(jackClient);
      csound->Message(csound, "Closed Jack client with result: %d\n", result);
      csound->DestroyThreadLock(csoundThreadLock);
      csound->Message(csound, "Destroyed Csound thread lock.\n");
    }
    csound->Message(csound, "ENDED JackState::close().\n");
    return result;
  }
  int processJack(jack_nframes_t frames)
  {
    // We must call PerformKsmps here ONLY after the original
    // Csound performance thread has been put to sleep.
    int result = 0;
    if (jackActive && !csoundActive) {
      // Enqueue any MIDI messages pending in input ports.
      midiInputQueue.clear();
      for (std::map<std::string, jack_port_t *>::iterator it = midiInPorts.begin();
	   it != midiInPorts.end();
	   ++it) {
	jack_port_t *midiinport = it->second;
	void *portbuffer = jack_port_get_buffer(midiinport, jackFramesPerTick);
	if (portbuffer) {
	  jack_nframes_t eventN = jack_midi_get_event_count(portbuffer);
	  for (jack_nframes_t eventI = 0; eventI < eventN; ++eventI) {
	    jack_midi_event_t event;
	    int result = jack_midi_event_get(&event, portbuffer, eventI);
	    if (result == 0) {
	      for (size_t i = 0; i < event.size; ++i) {
		midiInputQueue.push_back(event.buffer[i]);
	      }
	    }
	  }
	}
      }
      result = csound->PerformKsmps(csound);
      // We break here when Csound has finished performing.
      if (result) {
	csound->NotifyThreadLock(csoundThreadLock);
	csound->Message(csound, "Notified Csound thread lock.\n");
	csoundStop(csound);
	if (jackActive) {
	  close();
	}
      }
    }
    return result;
  }
  int processCsound()
  {
    // Here we must wait once and only once, in order to put
    // the original Csound processing thread to sleep -- 
    // but we must NOT put the Jack processing callback 
    // to sleep when it comes here!
    if (jackActive && csoundActive) {
      csoundActive = false;
      csound->Message(csound, 
		      "Put to sleep Csound performance thread %p at frame %d.\n", 
		      pthread_self(), 
		      jack_last_frame_time(jackClient));
      csound->WaitThreadLockNoTimeout(csoundThreadLock);
      csound->Message(csound, 
		      "Woke up Csound performance thread %p at frame %d.\n", 
		      pthread_self(), 
		      jack_last_frame_time(jackClient));
    }
    return 1;
  }
};

static JackState *getJackState(CSOUND *csound)
{
  return jackStatesForCsoundInstances[csound];
}

static int JackProcessCallback_(jack_nframes_t frames, 
				void *data)
{
  return ((JackState *)data)->processJack(frames);
}

static void SenseEventCallback_(CSOUND * csound, 
				void *data)
{
  ((JackState *)data)->processCsound();
}

static int midiDeviceOpen_(CSOUND *csound, 
			   void **userData,
			   const char *devName)
{
  *userData = getJackState(csound);
  return 0;
}

/** 
 * Dispatch any MIDI channel messages that have
 * been read from Jack MIDI input ports.
 */
static int midiRead_(CSOUND *csound, 
		     void *userData,
		     unsigned char *midiData, 
		     int nbytes)
{
  JackState *jackState = (JackState *)userData;
  int readSize = std::min(int(nbytes), int(jackState->midiInputQueue.size()));
  for (size_t i = 0; i < readSize; ++i) {
    midiData[i] = jackState->midiInputQueue.front();
    jackState->midiInputQueue.pop_front();
  }
  return readSize;
}

struct JackInit : public OpcodeBase<JackInit>
{
  MYFLT *ServerName;
  MYFLT *SclientName;
  const char *serverName;
  const char *clientName;
  JackState *jackState;
  int init(CSOUND *csound)
  {
    serverName = csound->strarg2name(csound,
				     (char *) 0,
				     ServerName,
				     (char *)"default",
				     (int) csound->GetInputArgSMask(this));
    clientName = csound->strarg2name(csound,
				     (char *) 0,
				     SclientName,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jackState = new JackState(csound, serverName, clientName);
    return OK;
  }
};

struct JackInfo : public OpcodeBase<JackInfo>
{
  JackState *jackState;
  int init(CSOUND *csound)
  {
    jackState = getJackState(csound);
    log(csound, "Jack information for client: %s\n", jackState->clientName);
    log(csound, "  Daemon name:               %s\n", jackState->serverName);
    log(csound, "  Frames per second:         %d\n", jackState->jackFramesPerSecond);
    log(csound, "  Frames per period:         %d\n", jackState->jackFramesPerTick);
    const char **ports = jack_get_ports(jackState->jackClient, 0, 0, 0);
    if (ports) {
      log(csound, "  Ports and connections:\n");
      for (size_t i = 0; ports[i]; ++i) {
	const char *PortName = ports[i];
	jack_port_t *port = jack_port_by_name(jackState->jackClient, PortName);
	int flags = jack_port_flags(port);
	const char *type = jack_port_type(port);
	const char *portType = "      ";
	if ((flags & JackPortIsOutput) == JackPortIsOutput) {
	  portType = "Output";
	} else if ((flags & JackPortIsInput) == JackPortIsInput) {
	  portType = "Input ";
	}
	log(csound, "    %3d:   %s   %-25s  %s\n", (i+1), portType, type, (PortName ? PortName : "(no name)"));
	const char **connections = jack_port_get_all_connections(jackState->jackClient, port);
	if (connections) {
	  for (size_t j = 0; connections[j]; ++j) {
	    if ((jack_port_flags(port) & JackPortIsOutput) == JackPortIsOutput) {
	      log(csound, "           Sends to:                           >> %s\n", connections[j]);
	    } else {
	      log(csound, "           Receives from:                      << %s\n", connections[j]);
	    }
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
    int result = jack_set_freewheel(jackState->jackClient, freewheel);
    if (result) {
      warn(csound, "Failed to set Jack freewheeling mode to \"%s\": error %d.\n", (freewheel ? "on" : "off"), result);
    } else {
      log(csound, "Set Jack freewheeling mode to \"%s\".\n", (freewheel ? "on" : "off"));
    }
    return result;
  }
};

struct JackOn : public OpcodeBase<JackOn>
{
  MYFLT *jon;
  JackState *jackState;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    jackState->jackActive = (char) *jon;
    log(csound, "Turned Jack connections \"%s\".\n", (jackState->jackActive ? "on" : "off"));
    return result;
  }
};

struct JackAudioInConnect : public OpcodeBase<JackAudioInConnect>
{
  // Out.
  // Ins.
  MYFLT *SexternalPortName;
  MYFLT *ScsoundPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    clientName = jack_get_client_name(jackState->jackClient);
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
    externalPortName = csound->strarg2name(csound,
				     (char *) 0,
				     SexternalPortName,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->jackClient, csoundFullPortName);
    if (!port_) {
      csoundPort = jack_port_register(jackState->jackClient, csoundPortName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
      if (csoundPort) {
	log(csound, "Created port \"%s\".\n", csoundFullPortName);
      } else {
	warn(csound, "Could not create port \"%s\".\n", csoundFullPortName);
      }
      externalPort = jack_port_by_name(jackState->jackClient, externalPortName);
      result = jack_connect(jackState->jackClient, jack_port_name(externalPort), jack_port_name(csoundPort));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    externalPortName, 
	    csoundFullPortName);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     externalPortName, 
	     csoundFullPortName,
	     result);
	return result;
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    externalPortName, 
	    csoundFullPortName);
      }
      jackState->audioInPorts[csoundPortName] = csoundPort;
    }
    return result;
  }
};

struct JackAudioIn : public OpcodeBase<JackAudioIn>
{
  // Out.
  MYFLT *asignal;
  // Ins.
  MYFLT *ScsoundPortName;
  // State.
  const char *csoundPortName;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    csoundFramesPerTick = jackState->csoundFramesPerTick;
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    csoundPort = jackState->audioInPorts[csoundPortName];
    return result;
  }
  int audio(CSOUND *csound)
  {
    jack_default_audio_sample_t *buffer = (jack_default_audio_sample_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
    for (size_t frame = 0; frame < csoundFramesPerTick; ++frame) {
      asignal[frame] = buffer[frame];
    }
    return OK;
  }
};

struct JackAudioOutConnect : public OpcodeBase<JackAudioOutConnect>
{
  // No outs.
  // Ins.
  MYFLT *ScsoundPortName;
  MYFLT *SexternalPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientName = jack_get_client_name(jackState->jackClient);
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
    externalPortName = csound->strarg2name(csound,
				     (char *) 0,
				     SexternalPortName,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->jackClient, csoundFullPortName);
    if (!port_) {
      csoundPort = jack_port_register(jackState->jackClient, csoundPortName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
      if (csoundPort) {
	log(csound, "Created port \"%s\".\n", csoundFullPortName);
      } else {
	warn(csound, "Could not create port \"%s\".\n", csoundFullPortName);
      }
      externalPort = jack_port_by_name(jackState->jackClient, externalPortName);
      result = jack_connect(jackState->jackClient, jack_port_name(csoundPort), jack_port_name(externalPort));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    csoundFullPortName, 
	    externalPortName);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     csoundFullPortName, 
	     externalPortName, 
	     result);
	return result;
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    csoundFullPortName, 
	    externalPortName);
      }
      jackState->audioOutPorts[csoundPortName] = csoundPort;
    }
    return result;
  }
};

struct JackAudioOut : public OpcodeBase<JackAudioOut>
{
  // No outs.
  // Ins.
  MYFLT *ScsoundPortName;
  MYFLT *asignal;
  // State.
  const char *csoundPortName;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    csoundFramesPerTick = jackState->csoundFramesPerTick;
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    csoundPort = jackState->audioOutPorts[csoundPortName];
    return result;
  }
  int audio(CSOUND *csound)
  {
    jack_default_audio_sample_t *buffer = (jack_default_audio_sample_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
    for (size_t frame = 0; frame < csoundFramesPerTick; ++frame) {
      buffer[frame] += asignal[frame];
    }
    return OK;
  }
};

struct JackMidiInConnect : public OpcodeBase<JackMidiInConnect>
{
  // No outs.
  // Ins.
  MYFLT *ScsoundPortName;
  MYFLT *SexternalPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientName = jack_get_client_name(jackState->jackClient);
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
    externalPortName = csound->strarg2name(csound,
				     (char *) 0,
				     SexternalPortName,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->jackClient, csoundFullPortName);
    if (!port_) {
      csoundPort = jack_port_register(jackState->jackClient, csoundPortName, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
      if (csoundPort) {
	log(csound, "Created port \"%s\".\n", csoundFullPortName);
      } else {
	warn(csound, "Could not create port \"%s\".\n", csoundFullPortName);
      }
      externalPort = jack_port_by_name(jackState->jackClient, externalPortName);
      result = jack_connect(jackState->jackClient, jack_port_name(csoundPort), jack_port_name(externalPort));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    externalPortName,
	    csoundFullPortName); 
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     externalPortName,
	     csoundFullPortName, 
	     result);
	return result;
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    externalPortName,
	    csoundFullPortName); 
      }
      jackState->midiInPorts[csoundPortName] = csoundPort;
    }
    return result;
  }
};

struct JackMidiOutConnect : public OpcodeBase<JackMidiOutConnect>
{
  // No outs.
  // Ins.
  MYFLT *ScsoundPortName;
  MYFLT *SexternalPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
    int result = OK;
    frames = csound->GetKsmps(csound);
    jackState = getJackState(csound);
    clientName = jack_get_client_name(jackState->jackClient);
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
    externalPortName = csound->strarg2name(csound,
				     (char *) 0,
				     SexternalPortName,
				     (char *)"csound",
				     (int) csound->GetInputArgSMask(this));    
    jack_port_t *port_ = jack_port_by_name(jackState->jackClient, csoundFullPortName);
    if (!port_) {
      csoundPort = jack_port_register(jackState->jackClient, csoundPortName, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
      if (csoundPort) {
	log(csound, "Created port \"%s\".\n", csoundFullPortName);
      } else {
	warn(csound, "Could not create port \"%s\".\n", csoundFullPortName);
      }
      externalPort = jack_port_by_name(jackState->jackClient, externalPortName);
      result = jack_connect(jackState->jackClient, jack_port_name(csoundPort), jack_port_name(externalPort));
      if (result == EEXIST) {
	log(csound, 
	    "Connection from \"%s\" to \"%s\" already exists.\n", 
	    csoundFullPortName, 
	    externalPortName);
      } else if (result) {
	warn(csound, 
	     "Could not create connection from \"%s\" to \"%s\": status %d.\n", 
	     csoundFullPortName, 
	     externalPortName, 
	     result);
	return result;
      } else {
	log(csound, 
	    "Created connection from \"%s\" to \"%s\".\n", 
	    csoundFullPortName, 
	    externalPortName);
      }
      jackState->midiOutPorts[csoundPortName] = csoundPort;
    }
    return result;
  }
};

struct JackMidiOut : public OpcodeBase<JackMidiOut>
{
  // No outs.
  // Ins.
  MYFLT *ScsoundPortName;
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
  const char *csoundPortName;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    csoundFramesPerTick = jackState->csoundFramesPerTick;
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    csoundPort = jackState->midiOutPorts[csoundPortName];
    jackState->opcodesForMidiOutPorts[csoundPort].push_back(this);
    priorstatus = -1;
    priorchannel = -1;
    priordata1 = -1;
    priordata2 = -1;
    return result;
  }
  int kontrol(CSOUND *csound)
  {
    int result = OK;
    status = *kstatus;
    channel = *kchannel;
    channel--;
    data1 = *kdata1;
    data2 = *kdata2;
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
      buffer = (jack_midi_data_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
      if (this == jackState->opcodesForMidiOutPorts[csoundPort].front()) {
	jack_midi_clear_buffer(buffer);
      }
      jack_midi_data_t *data = jack_midi_event_reserve(buffer, 0, dataSize);
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
  MYFLT *ScsoundPortName;
  MYFLT *ichannel;
  MYFLT *ikey;
  MYFLT *ivelocity;
  char status;
  char channel;
  char key;
  char velocity;
  // State.
  const char *csoundPortName;
  JackState *jackState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
    int result = OK;
    jackState = getJackState(csound);
    csoundFramesPerTick = jackState->csoundFramesPerTick;
    csoundPortName = csound->strarg2name(csound,
				   (char *)0,
				   ScsoundPortName,
				   (char *)"",
				   (int) csound->GetInputArgSMask(this));
    csoundPort = jackState->midiOutPorts[csoundPortName];
    jackState->opcodesForMidiOutPorts[csoundPort].push_back(this);
    status = 144;
    channel = (char) *ichannel;
    channel--;
    key = (char) *ikey;
    velocity = (char) *ivelocity;
    buffer = (jack_midi_data_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
    if (this == jackState->opcodesForMidiOutPorts[csoundPort].front()) {
      jack_midi_clear_buffer(buffer);
    }
    jack_midi_data_t *data = jack_midi_event_reserve(buffer, 0, 3);
    data[0] = (status + channel);
    data[1] = key;
    data[2] = velocity;  
    //log(csound, "noteon:  %3d %3d %3d\n", data[0], data[1], data[2]);
    return result;
  }
  int noteoff(CSOUND *csound)
  {
    int result = OK;
    buffer = (jack_midi_data_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
    if (this == jackState->opcodesForMidiOutPorts[csoundPort].front()) {
      jack_midi_clear_buffer(buffer);
    }
    jack_midi_data_t *data = jack_midi_event_reserve(buffer, 0, 3);
    data[0] = (status + channel);
    data[1] = key;
    data[2] = 0;  
    //log(csound, "noteoff: %3d %3d %3d\n", data[0], data[1], data[2]);
    return result;
  }
};

extern "C"
{
  static OENTRY oentries[] = {
    {
      (char *)"JackInit",
      sizeof(JackInit),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackInit::init_,
      0,
      0
    },
    {
      (char *)"JackInfo",
      sizeof(JackInfo),
      1,
      (char *)"",
      (char *)"",
      (SUBR)&JackInfo::init_,
      0,
      0
    },
    {
      (char *)"JackFreewheel",
      sizeof(JackFreewheel),
      1,
      (char *)"",
      (char *)"i",
      (SUBR)&JackFreewheel::init_,
      0,
      0
    },
    {
      (char *)"JackOn",
      sizeof(JackOn),
      1,
      (char *)"",
      (char *)"j",
      (SUBR)&JackOn::init_,
      0,
      0
    },
    {
      (char *)"JackAudioInConnect",
      sizeof(JackAudioInConnect),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackAudioInConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackAudioIn",
      sizeof(JackAudioIn),
      5,
      (char *)"a",
      (char *)"S",
      (SUBR)&JackAudioIn::init_,
      0,
      (SUBR)&JackAudioIn::audio_,
    },
    {
      (char *)"JackAudioOutConnect",
      sizeof(JackAudioOutConnect),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackAudioOutConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackAudioOut",
      sizeof(JackAudioOut),
      5,
      (char *)"",
      (char *)"Sa",
      (SUBR)&JackAudioOut::init_,
      0,
      (SUBR)&JackAudioOut::audio_,
    },
    {
      (char *)"JackMidiInConnect",
      sizeof(JackMidiInConnect),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackMidiInConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackMidiOutConnect",
      sizeof(JackMidiOutConnect),
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackMidiOutConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackMidiOut",
      sizeof(JackMidiOut),
      3,
      (char *)"",
      (char *)"Skkkj",
      (SUBR)&JackMidiOut::init_,
      (SUBR)&JackMidiOut::kontrol_,
      0,
    },
    {
      (char *)"JackNoteOut",
      sizeof(JackNoteOut),
      3,
      (char *)"",
      (char *)"Siii",
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
      csound->Message(csound, "jacko: CsoundModuleDestroy(%p)\n", csound);
      delete jackStatesForCsoundInstances[csound];
    }
    return result;
  }
}


