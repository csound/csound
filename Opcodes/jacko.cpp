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
 * JackoInit -- Initializes Csound as a Jack client.
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
 * opcode definition within the Csound orchestra.
 *
 * Syntax
 *
 * JackoInit SclientName, ServerName
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
 * If more than one instance of Csound is using the Jack
 * opcodes at the same time, then each instance of Csound
 * must use a different client name.
 *
 *
 * JackoInfo -- Prints information about the Jack system.
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
 * JackoInfo
 *
 * Initialization
 *
 * May be called any number of times in the orchestra header,
 * for example both before and after creating Jack ports
 * in the Csound orchestra header.
 *
 *
 * JackoFreewheel -- Turns freewheeling mode on or off.
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
 * JackoFreewheel [ienabled]
 *
 * Initialization
 *
 * ienabled -- Turns freewheeling on (the default) or off.
 *
 *
 * JackoAudioInConnect -- Creates an audio connection
 *                        from a Jack port to Csound.
 *
 * Description
 *
 * In the orchestra header, creates an audio connection
 * from an external Jack audio output port to a
 * Jack audio input port inside this instance of Csound.
 *
 * Syntax
 *
 * JackoAudioInConnect SexternalPortName, ScsoundPortName
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
 * The actual audio must be read with the JackoAudioIn opcode.
 *
 *
 * JackoAudioOutConnect -- Creates an audio connection
 *                         from Csound to a Jack port.
 *
 * Description
 *
 * In the orchestra header, creates an audio connection
 * from a Jack audio output port inside this instance
 * of Csound to an external Jack audio input port.
 *
 * Syntax
 *
 * JackoAudioOutConnect ScsoundPortName, SexternalPortName
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
 * The actual audio must be written with the JackoAudioOut
 * opcode.
 *
 *
 * JackoMidiInConnect -- Creates a MIDI connection from
 *                       Jack to Csound.
 *
 * Description
 *
 * Creates a MIDI connection from an external Jack MIDI
 * output port to this instance of Csound.
 *
 * Syntax
 *
 * JackoMidiInConnect SexternalPortName, ScsoundPortName
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
 * -M0 -+rtmidi=null Csound command-line options.
 * Can be used in with the MIDI inter-operability
 * command-line options and/or opcodes to enable the
 * use of ordinary Csound instrument definitions to
 * render external scores or MIDI sequences.
 *
 * Note that Csound can connect to ALSA ports through Jack,
 * but in this case you will have to identify the port by
 * its alias in the JackInfo printout.
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
 * JackoMidiOutConnect -- Creates a MIDI connection from
 *                        Csound to Jack.
 *
 * Description
 *
 * In the orchestra header, creates a connection
 * from a Jack MIDI output port inside this instance
 * of Csound to an external Jack MIDI input port.
 *
 * Syntax
 *
 * JackoMidiOutConnect ScsoundPortName, SexternalPortName
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
 * The actual MIDI data must be written with the JackoMidiOut
 * or JackoNoteOut opcodes.
 *
 *
 * JackoOn -- Enables or disables all Jack ports.
 *
 * Description
 *
 * After all Jack connections have been created, enables
 * or disables all Jack input and output opcodes
 * inside this instance of Csound to read or write data.
 *
 * Syntax
 *
 * JackoOn [iactive]
 *
 * Initialization
 *
 * iactive -- A flag that turns the ports on (the default)
 * or off.
 *
 *
 * JackoAudioIn -- Receives an audio signal from a Jack port.
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
 * asignal JackoAudioIn ScsoundPortName
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
 * JackoAudioOut -- Sends an audio signal to a Jack port.
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
 * JackoAudioOut ScsoundPortName, asignal
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
 * JackoMidiOut -- Sends a MIDI channel message to a
 *                 Jack port.
 *
 * Description
 *
 * Sends a MIDI channel message to a Jack MIDI output port
 * inside this instance of Csound, and in turn to its
 * connected external Jack MIDI input port.
 *
 * Syntax
 *
 * JackoMidiOut ScsoundPortName, kstatus, kchannel, kdata1[, kdata2]
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
 * JackoNoteOut -- Send one note to a Jack MIDI port.
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
 * JackoNoteOut ScsoundPortName, ichannel, ikey, ivelocity
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
 * JackoTransport -- Control the Jack transport.
 *
 * Description
 *
 * Starts, stops, or repositions the Jack transport.
 * This is useful, e.g., for starting an external sequencer
 * playing to send MIDI messages to Csound.
 *
 * Syntax
 *
 * JackoTransport kcommand, [kposition]
 *
 * Performance
 *
 * kcommand -- 0 means "no action", 1 starts the transport,
 * 2 stops the transport, and 3 positions the transport
 * to kposition seconds from the beginning of performance
 * (i.e. time 0 in the score).
 *
 * This opcode can be used at init time or during performance.
 *
 * The granularity of timing is Csound's kperiod.
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
#include <cstring>
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

struct JackoInit;
struct JackoInfo;
struct JackoFreewheel;
struct JackoAudioIn;
struct JackoAudioOut;
struct JackoMidiOut;
struct JackoNoteOut;
struct JackoTransport;
struct JackoState;

#if 0
static JackoState *getJackoState(CSOUND *csound);
static void *closeRoutine(void *userdata);
#endif

static std::map<CSOUND *, JackoState *> jackoStatesForCsoundInstances;
static void SenseEventCallback_(CSOUND *csound,
                                void *data);
static int JackProcessCallback_(jack_nframes_t frames,
                                void *data);
static int midiDeviceOpen_(CSOUND *csound,
                           void **userData,
                           const char *devName);

static int midiRead_(CSOUND *csound,
                     void *userData,
                     unsigned char *midiData,
                     int nbytes);

/**
 * Manages all state relevant to the global
 * interaction between Jack and Csound for a particular
 * client and instance of Csound. The actual reading
 * and writing of data is done by the input and output
 * opcodes.
 */
struct JackoState
{
  CSOUND *csound;
  const char *serverName;
  const char *clientName;
  jack_client_t *jackClient;
  char jackInitialized;
  char jackActive;
  char csoundActive;
  jack_nframes_t csoundFramesPerTick;
  jack_nframes_t jackFramesPerTick;
  jack_nframes_t csoundFramesPerSecond;
  jack_nframes_t jackFramesPerSecond;
  jack_nframes_t jackFrameTime;
  std::map<std::string, jack_port_t *> audioInPorts;
  std::map<std::string, jack_port_t *> audioOutPorts;
  std::map<std::string, jack_port_t *> midiInPorts;
  std::map<std::string, jack_port_t *> midiOutPorts;
  std::list<unsigned char> midiInputQueue;
  jack_position_t jack_position;
  pthread_t closeThread;
  pthread_mutex_t conditionMutex;
  pthread_cond_t closeCondition;
  pthread_cond_t csoundCondition;
  JackoState(CSOUND *csound_, const char *serverName_, const char *clientName_) :
    csound(csound_),
    serverName(serverName_),
    clientName(clientName_),
    jackInitialized(false),
    jackActive(false),
    csoundActive(true)
  {
      int result = 0;
      csound = csound_;
      csoundFramesPerTick = csound->GetKsmps(csound);
      csoundFramesPerSecond = csound->GetSr(csound);
      result |= pthread_mutex_init(&conditionMutex, 0);
      result |= pthread_cond_init(&csoundCondition, 0);
      result |= pthread_cond_init(&closeCondition, 0);
      // Create a thread to run the close routine. It will immediately
      // block until it is signaled.
      result |= pthread_create(&closeThread, 0, &JackoState::closeRoutine_, this);
      std::memset(&jack_position, 0, sizeof(jack_position_t));
      jack_options_t jack_options = (jack_options_t) (JackServerName |
                                                      JackNoStartServer |
                                                      JackUseExactName);
      jack_status_t status = jack_status_t(0);
      jackClient = jack_client_open(clientName,
                                    jack_options,
                                    &status,
                                    serverName);
      if (!jackClient) {
        csound->Message(csound, Str("Could not create Jack client \"%s\" -- "
                                    "is Jack server \"%s\" running? Status: %d\n"),
                        clientName,
                        serverName,
                        status);
        csound->LongJmp(csound, 1);
      } else {
        csound->Message(csound,
                        Str("Created Jack client \"%s\" for Jack server \"%s\".\n"),
                        clientName, serverName);
      }
      jackFramesPerTick = jack_get_buffer_size(jackClient);
      if (csoundFramesPerTick != jackFramesPerTick) {
        csound->Message(csound,
                        Str("Jack buffer size %d != Csound ksmps %d, exiting...\n"),
                        jackFramesPerTick,
                        csoundFramesPerTick);
        csound->LongJmp(csound, 1);
      }
      jackFramesPerSecond = jack_get_sample_rate(jackClient);
      if (csoundFramesPerSecond != jackFramesPerSecond) {
        csound->Message(csound,
                        Str("Jack sampling rate %d != Csound sr %d, exiting...\n"),
                        jackFramesPerSecond,
                        csoundFramesPerSecond);
        csound->LongJmp(csound, 1);
      }
      jackoStatesForCsoundInstances[csound] = this;
      csound->RegisterSenseEventCallback(csound, SenseEventCallback_, this);
      result |= jack_set_process_callback(jackClient, JackProcessCallback_, this);
      result |= jack_activate(jackClient);
      if (!result) {
        csound->Message(csound,
                        Str("Activated Jack client \"%s\".\n"),
                        jack_get_client_name(jackClient));
      } else {
        csound->Message(csound,
                        Str("Failed to activate Jack client \"%s\": status %d.\n"),
                        jack_get_client_name(jackClient),
                        result);
        return;
      }
      csound->SetExternalMidiInOpenCallback(csound, midiDeviceOpen_);
      csound->SetExternalMidiReadCallback(csound, midiRead_);
      jackInitialized = true;
  }
  ~JackoState()
  {
      //int result = 0;           // This does NOTHING!
  }
  int close()
  {
      csound->Message(csound, Str("BEGAN JackoState::close()...\n"));
      int result = OK;
      // Try not to do thread related operations more than once...
      if (jackInitialized) {
        jackInitialized = false;
        jackActive = false;
        result = jack_deactivate(jackClient);
        for (std::map<std::string, jack_port_t *>
               ::iterator it = audioInPorts.begin();
             it != audioInPorts.end();
             ++it) {
          result = jack_port_unregister(jackClient, it->second);
        }
        for (std::map<std::string, jack_port_t *>
               ::iterator it = audioOutPorts.begin();
             it != audioOutPorts.end();
             ++it) {
          result = jack_port_unregister(jackClient, it->second);
        }
        for (std::map<std::string, jack_port_t *>
               ::iterator it = midiInPorts.begin();
             it != midiInPorts.end();
             ++it) {
          result = jack_port_unregister(jackClient, it->second);
        }
        for (std::map<std::string, jack_port_t *>
               ::iterator it = midiOutPorts.begin();
             it != midiOutPorts.end();
             ++it) {
          result = jack_port_unregister(jackClient, it->second);
        }
        result |= jack_client_close(jackClient);
        result |= pthread_cond_destroy(&csoundCondition);
        result |= pthread_cond_destroy(&closeCondition);
        result |= pthread_mutex_destroy(&conditionMutex);
        audioOutPorts.clear();
        audioInPorts.clear();
        midiInPorts.clear();
        midiOutPorts.clear();
      }
      csound->Message(csound, Str("ENDED JackoState::close().\n"));
      return result;
  }
  int processJack(jack_nframes_t frames)
  {
      // We must call PerformKsmps here ONLY after the original
      // Csound performance thread is waiting on its condition.
      int result = 0;
      jackFrameTime = jack_last_frame_time(jackClient);
      if (jackActive && !csoundActive) {
        // Enqueue any MIDI messages pending in input ports.
        for (std::map<std::string, jack_port_t *>
               ::iterator it = midiInPorts.begin();
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
        // Clear MIDI output buffers.
        for (std::map<std::string, jack_port_t *>
               ::iterator it = midiOutPorts.begin();
             it != midiOutPorts.end();
             ++it) {
          void *buffer = jack_port_get_buffer(it->second, jackFramesPerTick);
          jack_midi_clear_buffer(buffer);
        }
        result = csound->PerformKsmps(csound);
        // We break here when the Csound performance is complete,
        // and signal the original Csound performance thread to continue.
        if (result && jackActive) {
          csoundActive = true;
          jackActive = false;
          pthread_mutex_lock(&conditionMutex);
          pthread_cond_signal(&csoundCondition);
          pthread_mutex_unlock(&conditionMutex);
          return result;
        }
      }
      return result;
  }
  int processCsound()
  {
      int result = 0;
      // Here we must wait once and only once, in order to put
      // the original Csound processing thread to sleep --
      // but we must NOT put the Jack processing callback
      // to sleep when it comes here!
      if (jackActive && csoundActive) {
        csoundActive = false;
        // While Jack is processing, wait here.
        // The Jack process callback will then call csoundPerformKsmps
        // until the Csound performance is complete.
        result |= pthread_mutex_lock(&conditionMutex);
        result |= pthread_cond_wait(&csoundCondition, &conditionMutex);
        result |= pthread_mutex_unlock(&conditionMutex);
      }
      if (jackActive) {
        return 1;
      } else {
        // Then, when the Csound performance is complete,
        // we signal the close routine condition so that
        // Jack can be shut down and cleaned up in a separate
        // thread. Doing this inside the Jack process callback
        // takes too long and may cause other problems.
        result = pthread_mutex_lock(&conditionMutex);
        result |= pthread_cond_signal(&closeCondition);
        result |= pthread_mutex_unlock(&conditionMutex);
        return result;
      }
  }
  void *closeRoutine()
  {
      int result = 0;
      // Wait until signaled to actually shut down the Jack client.
      result = pthread_mutex_lock(&conditionMutex);
      result |= pthread_cond_wait(&closeCondition, &conditionMutex);
      result |= pthread_mutex_unlock(&conditionMutex);
      close();
      void *result_ = 0;
      memcpy(&result_, &result, std::min(sizeof(result), sizeof(result_)));
      return result_;
  }
  static void *closeRoutine_(void *userdata)
  {
      return ((JackoState *)userdata)->closeRoutine();
  }\
  void startTransport()
  {
      midiInputQueue.clear();
      jack_transport_start(jackClient);
  }
  void stopTransport()
  {
      jack_transport_stop(jackClient);
  }
  int positionTransport(double timeSeconds)
  {
      int result = OK;
      jack_position.frame_time = timeSeconds;
      midiInputQueue.clear();
      result = jack_transport_reposition(jackClient, &jack_position);
      return result;
  }
  /**
   * Return a MIDI output buffer,
   * clearing it if not yet cleared for this tick.
   */
  jack_midi_data_t *getMidiOutBuffer(jack_port_t *csoundPort)
  {
      jack_midi_data_t *buffer =
        (jack_midi_data_t *)jack_port_get_buffer(csoundPort, csoundFramesPerTick);
      return buffer;
  }
};

static JackoState *getJackoState(CSOUND *csound)
{
    return jackoStatesForCsoundInstances[csound];
}

static int JackProcessCallback_(jack_nframes_t frames,
                                void *data)
{
    return ((JackoState *)data)->processJack(frames);
}

static void SenseEventCallback_(CSOUND * csound,
                                void *data)
{
    ((JackoState *)data)->processCsound();
}

static int midiDeviceOpen_(CSOUND *csound,
                           void **userData,
                           const char *devName)
{
    *userData = getJackoState(csound);
    return 0;
}

/**
 * Dispatch any MIDI channel messages that have
 * been read from Jack MIDI input ports.
 */
static int midiRead_(CSOUND *csound,
                     void *userData,
                     unsigned char *midiData,
                     int midiN)
{
    JackoState *jackoState = (JackoState *)userData;
    int midiI = 0;
    while (!jackoState->midiInputQueue.empty() && midiI < midiN) {
      midiData[midiI] = jackoState->midiInputQueue.front();
      jackoState->midiInputQueue.pop_front();
      midiI++;
    }
    //if (midiI) {
    //  csound->Message(csound, "midiRead_: %d bytes.\n", midiI);
    //}
    return midiI;
}

struct JackoInit : public OpcodeBase<JackoInit>
{
  STRINGDAT *ServerName;
  STRINGDAT *SclientName;
  const char *serverName;
  const char *clientName;
  JackoState *jackoState;
  int init(CSOUND *csound)
  {
    serverName = csound->strarg2name(csound,
                                     (char *) 0,
                                     ServerName->data,
                                     (char *)"default",
                                     (int) 1);
    clientName = csound->strarg2name(csound,
                                     (char *) 0,
                                     SclientName->data,
                                     (char *)"csound",
                                     (int) 1);
    jackoState = new JackoState(csound, serverName, clientName);
    return OK;
  }
};

struct JackoInfo : public OpcodeBase<JackoInfo>
{
  JackoState *jackoState;
  int init(CSOUND *csound)
  {
    jackoState = getJackoState(csound);
    log(csound, "Jack information for client: %s\n", jackoState->clientName);
    log(csound, "  Daemon name:               %s\n", jackoState->serverName);
    log(csound, "  Frames per second:         %d\n",
        jackoState->jackFramesPerSecond);
    log(csound, "  Frames per period:         %d\n",
        jackoState->jackFramesPerTick);
    const char **ports = jack_get_ports(jackoState->jackClient, 0, 0, 0);
    if (ports) {
      log(csound, "  Ports and connections:\n");
      for (size_t i = 0; ports[i]; ++i) {
        const char *PortName = ports[i];
        jack_port_t *port = jack_port_by_name(jackoState->jackClient, PortName);
        int flags = jack_port_flags(port);
        const char *type = jack_port_type(port);
        const char *portType = "      ";
        if ((flags & JackPortIsOutput) == JackPortIsOutput) {
          portType = "Output";
        } else if ((flags & JackPortIsInput) == JackPortIsInput) {
          portType = "Input ";
        }
        log(csound, "    %3d:   %s   %-25s  %s\n",
            (i+1), portType, type, (PortName ? PortName : "(no name)"));
        char alias1[0x100];
        char alias2[0x100];
        char * const aliases[2] = {alias1, alias2};
        size_t aliasN = jack_port_get_aliases(port, aliases);
        for (size_t aliasI = 0; aliasI < aliasN; ++aliasI) {
          log(csound, "           Alias: %s\n", aliases[aliasI]);
        }
        const char **connections =
          jack_port_get_all_connections(jackoState->jackClient, port);
        if (connections) {
          for (size_t j = 0; connections[j]; ++j) {
            if ((jack_port_flags(port) & JackPortIsOutput) == JackPortIsOutput) {
              log(csound, "           Sends to:                           >> %s\n",
                  connections[j]);
            } else {
              log(csound, "           Receives from:                      << %s\n",
                  connections[j]);
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

struct JackoFreewheel : public OpcodeBase<JackoFreewheel>
{
  MYFLT *ifreewheel;
  JackoState *jackoState;
  int init(CSOUND *csound)
  {
    jackoState = getJackoState(csound);
    int freewheel = (int) *ifreewheel;
    int result = jack_set_freewheel(jackoState->jackClient, freewheel);
    if (result) {
      warn(csound,
           Str("Failed to set Jack freewheeling mode to \"%s\": error %d.\n"),
           (freewheel ? "on" : "off"), result);
    } else {
      log(csound, Str("Set Jack freewheeling mode to \"%s\".\n"),
          (freewheel ? "on" : "off"));
    }
    return result;
  }
};

struct JackoOn : public OpcodeBase<JackoOn>
{
  MYFLT *jon;
  JackoState *jackoState;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      jackoState->jackActive = (char) *jon;
      log(csound, Str("Turned Jack connections \"%s\".\n"),
          (jackoState->jackActive ? "on" : "off"));
      return result;
  }
};

struct JackoAudioInConnect : public OpcodeBase<JackoAudioInConnect>
{
  // Out.
  // Ins.
  STRINGDAT *SexternalPortName;
  STRINGDAT *ScsoundPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      clientName = jack_get_client_name(jackoState->jackClient);
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
      externalPortName = csound->strarg2name(csound,
                                             (char *) 0,
                                             SexternalPortName->data,
                                             (char *)"csound",
                                             (int) 1);
      csoundPort = jack_port_by_name(jackoState->jackClient, csoundFullPortName);
      if (!csoundPort) {
        csoundPort = jack_port_register(jackoState->jackClient,
                                        csoundPortName, JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsInput, 0);
        if (csoundPort) {
          log(csound, "Created Jack port \"%s\".\n", csoundFullPortName);
        } else {
          warn(csound, Str("Could not create Jack port \"%s\".\n"),
               csoundFullPortName);
        }
      }
      externalPort = jack_port_by_name(jackoState->jackClient, externalPortName);
      result = jack_connect(jackoState->jackClient, jack_port_name(externalPort),
                            jack_port_name(csoundPort));
      if (result == EEXIST) {
        log(csound,
            "Connection from \"%s\" to \"%s\" already exists.\n",
            externalPortName,
            csoundFullPortName);
      } else if (result) {
        warn(csound,
             Str("Could not create Jack connection from \"%s\" to \"%s\": "
                 "status %d.\n"),
             externalPortName,
             csoundFullPortName,
             result);
        return result;
      } else {
        log(csound,
            "Created Jack connection from \"%s\" to \"%s\".\n",
            externalPortName,
            csoundFullPortName);
      }
      jackoState->audioInPorts[csoundPortName] = csoundPort;
      return result;
  }
};

struct JackoAudioIn : public OpcodeBase<JackoAudioIn>
{
  // Out.
  MYFLT *asignal;
  // Ins.
  STRINGDAT *ScsoundPortName;
  // State.
  const char *csoundPortName;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      csoundFramesPerTick = jackoState->csoundFramesPerTick;
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      csoundPort = jackoState->audioInPorts[csoundPortName];
      return result;
  }
  int audio(CSOUND *csound)
  {
      jack_default_audio_sample_t *buffer =
        (jack_default_audio_sample_t *)jack_port_get_buffer(csoundPort,
                                                            csoundFramesPerTick);
      for (size_t frame = 0; frame < csoundFramesPerTick; ++frame) {
        asignal[frame] = buffer[frame];
      }
      return OK;
  }
};

struct JackoAudioOutConnect : public OpcodeBase<JackoAudioOutConnect>
{
  // No outs.
  // Ins.
  STRINGDAT *ScsoundPortName;
  STRINGDAT *SexternalPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
      int result = OK;
      frames = opds.insdshead->ksmps;
      jackoState = getJackoState(csound);
      clientName = jack_get_client_name(jackoState->jackClient);
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
      externalPortName = csound->strarg2name(csound,
                                             (char *) 0,
                                             SexternalPortName->data,
                                             (char *)"csound",
                                             (int) 1);
      csoundPort = jack_port_by_name(jackoState->jackClient, csoundFullPortName);
      if (!csoundPort) {
        csoundPort = jack_port_register(jackoState->jackClient,
                                        csoundPortName, JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);
        if (csoundPort) {
          log(csound, "Created Jack port \"%s\".\n", csoundFullPortName);
        } else {
          warn(csound, Str("Could not create Jack port \"%s\".\n"),
               csoundFullPortName);
        }
      }
      externalPort = jack_port_by_name(jackoState->jackClient, externalPortName);
      result = jack_connect(jackoState->jackClient,
                            jack_port_name(csoundPort),
                            jack_port_name(externalPort));
      if (result == EEXIST) {
        log(csound,
            "Connection from \"%s\" to \"%s\" already exists.\n",
            csoundFullPortName,
            externalPortName);
      } else if (result) {
        warn(csound,
             Str("Could not create Jack connection from \"%s\" to \"%s\": "
                 "status %d.\n"),
             csoundFullPortName,
             externalPortName,
             result);
        return result;
      } else {
        log(csound,
            "Created Jack connection from \"%s\" to \"%s\".\n",
            csoundFullPortName,
            externalPortName);
      }
      jackoState->audioOutPorts[csoundPortName] = csoundPort;
      return result;
  }
};

struct JackoAudioOut : public OpcodeBase<JackoAudioOut>
{
  // No outs.
  // Ins.
  STRINGDAT *ScsoundPortName;
  MYFLT *asignal;
  // State.
  const char *csoundPortName;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      csoundFramesPerTick = jackoState->csoundFramesPerTick;
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      csoundPort = jackoState->audioOutPorts[csoundPortName];
      return result;
  }
  int audio(CSOUND *csound)
  {
      jack_default_audio_sample_t *buffer =
        (jack_default_audio_sample_t *)jack_port_get_buffer(csoundPort,
                                                            csoundFramesPerTick);
      for (size_t frame = 0; frame < csoundFramesPerTick; ++frame) {
        buffer[frame] = asignal[frame];
      }
      return OK;
  }
};

struct JackoMidiInConnect : public OpcodeBase<JackoMidiInConnect>
{
  // No outs.
  // Ins.
  STRINGDAT *SexternalPortName;
  STRINGDAT *ScsoundPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
      int result = OK;
      frames = opds.insdshead->ksmps;
      jackoState = getJackoState(csound);
      clientName = jack_get_client_name(jackoState->jackClient);
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
      externalPortName = csound->strarg2name(csound,
                                             (char *) 0,
                                             SexternalPortName->data,
                                             (char *)"csound",
                                             (int) 1);
      csoundPort = jack_port_by_name(jackoState->jackClient, csoundFullPortName);
      if (!csoundPort) {
        csoundPort = jack_port_register(jackoState->jackClient, csoundPortName,
                                        JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (csoundPort) {
          log(csound, "Created Jack port \"%s\".\n", csoundFullPortName);
        } else {
          warn(csound, Str("Could not create Jack port \"%s\".\n"),
               csoundFullPortName);
        }
      }
      externalPort = jack_port_by_name(jackoState->jackClient, externalPortName);
      result = jack_connect(jackoState->jackClient,
                            jack_port_name(externalPort),
                            jack_port_name(csoundPort));
      if (result == EEXIST) {
        log(csound,
            "Connection from \"%s\" to \"%s\" already exists.\n",
            externalPortName,
            csoundFullPortName);
      } else if (result) {
        warn(csound,
             Str("Could not create Jack connection from \"%s\" to \"%s\": "
                 "status %d.\n"),
             externalPortName,
             csoundFullPortName,
             result);
        return result;
      } else {
        log(csound,
            "Created Jack connection from \"%s\" to \"%s\".\n",
            externalPortName,
            csoundFullPortName);
      }
      jackoState->midiInPorts[csoundPortName] = csoundPort;
      return result;
  }
};

struct JackoMidiOutConnect : public OpcodeBase<JackoMidiOutConnect>
{
  // No outs.
  // Ins.
  STRINGDAT *ScsoundPortName;
  STRINGDAT *SexternalPortName;
  // State.
  const char *csoundPortName;
  char csoundFullPortName[0x100];
  const char *externalPortName;
  const char *clientName;
  size_t frames;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_port_t *externalPort;
  int init(CSOUND *csound)
  {
      int result = OK;
      frames = opds.insdshead->ksmps;
      jackoState = getJackoState(csound);
      clientName = jack_get_client_name(jackoState->jackClient);
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      std::sprintf(csoundFullPortName, "%s:%s", clientName, csoundPortName);
      externalPortName = csound->strarg2name(csound,
                                             (char *) 0,
                                             SexternalPortName->data,
                                             (char *)"csound",
                                             (int) 1);
      csoundPort = jack_port_by_name(jackoState->jackClient, csoundFullPortName);
      if (!csoundPort) {
        csoundPort = jack_port_register(jackoState->jackClient,
                                        csoundPortName, JACK_DEFAULT_MIDI_TYPE,
                                        JackPortIsOutput, 0);
        if (csoundPort) {
          log(csound, "Created Jack port \"%s\".\n", csoundFullPortName);
        } else {
          warn(csound, Str("Could not create Jack port \"%s\".\n"),
               csoundFullPortName);
        }
      }
      externalPort = jack_port_by_name(jackoState->jackClient, externalPortName);
      result = jack_connect(jackoState->jackClient, jack_port_name(csoundPort),
                            jack_port_name(externalPort));
      if (result == EEXIST) {
        log(csound,
            "Connection from \"%s\" to \"%s\" already exists.\n",
            csoundFullPortName,
            externalPortName);
      } else if (result) {
        warn(csound,
             Str("Could not create Jack connection from \"%s\" to \"%s\": "
                 "status %d.\n"),
             csoundFullPortName,
             externalPortName,
             result);
        return result;
      } else {
        log(csound,
            "Created Jack connection from \"%s\" to \"%s\".\n",
            csoundFullPortName,
            externalPortName);
      }
      jackoState->midiOutPorts[csoundPortName] = csoundPort;
      return result;
  }
};

struct JackoMidiOut : public OpcodeBase<JackoMidiOut>
{
  // No outs.
  // Ins.
  STRINGDAT *ScsoundPortName;
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
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      csoundFramesPerTick = jackoState->csoundFramesPerTick;
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      csoundPort = jackoState->midiOutPorts[csoundPortName];
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
        buffer = jackoState->getMidiOutBuffer(csoundPort);
        jack_midi_data_t *data = jack_midi_event_reserve(buffer, 0, dataSize);
        data[0] = (status + channel);
        data[1] = data1;
        if (data2 != -1) {
          data[2] = data2;
          //log(csound, "MIDI:  %3d %3d %3d\n", data[0], data[1], data[2]);
        } else {
          //log(csound, "MIDI:  %3d %3d\n", data[0], data[1]);
        }
      }
      priorstatus = status;
      priorchannel = channel;
      priordata1 = data1;
      priordata2 = data2;
      return result;
  }
};

struct JackoNoteOut : public OpcodeNoteoffBase<JackoNoteOut>
{
  // No outs.
  // Ins.
  STRINGDAT *ScsoundPortName;
  MYFLT *ichannel;
  MYFLT *ikey;
  MYFLT *ivelocity;
  char status;
  char channel;
  char key;
  char velocity;
  // State.
  const char *csoundPortName;
  JackoState *jackoState;
  jack_port_t *csoundPort;
  jack_nframes_t csoundFramesPerTick;
  jack_midi_data_t *buffer;
  int init(CSOUND *csound)
  {
      int result = OK;
      jackoState = getJackoState(csound);
      csoundFramesPerTick = jackoState->csoundFramesPerTick;
      csoundPortName = csound->strarg2name(csound,
                                           (char *)0,
                                           ScsoundPortName->data,
                                           (char *)"",
                                           (int) 1);
      csoundPort = jackoState->midiOutPorts[csoundPortName];
      status = 144;
      channel = (char) *ichannel;
      key = (char) *ikey;
      velocity = (char) *ivelocity;
      buffer = jackoState->getMidiOutBuffer(csoundPort);
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
      buffer = jackoState->getMidiOutBuffer(csoundPort);
      jack_midi_data_t *data = jack_midi_event_reserve(buffer, 0, 3);
      data[0] = (status + channel);
      data[1] = key;
      data[2] = 0;
      //log(csound, "noteoff: %3d %3d %3d\n", data[0], data[1], data[2]);
      return result;
  }
};

struct JackoTransport : public OpcodeBase<JackoTransport>
{
  // Outs.
  // Ins.
  MYFLT *kcommand;
  MYFLT *Oposition;
  // State.
  JackoState *jackoState;
  int command;
  int priorCommand;
  double positionSeconds;
  double priorPositionSeconds;
  int init(CSOUND *csound)
  {
      jackoState = getJackoState(csound);
      priorCommand = -1;
      priorPositionSeconds = 0.0;
      return kontrol(csound);
  }
  int kontrol(CSOUND *csound)
  {
      int result = OK;
      command = int(*kcommand);
      positionSeconds = double(*Oposition);
      if (command) {
        if (command != priorCommand) {
          priorCommand = command;
          switch(command) {
          case 1:
            result = jackoState->positionTransport(0.0);
            jackoState->startTransport();
            log(csound, "Started Jack transport.\n");
            break;
        case 2:
            jackoState->stopTransport();
            log(csound, "Stopped Jack transport.\n");
            break;
        case 3:
            if (positionSeconds != priorPositionSeconds) {
              priorPositionSeconds = positionSeconds;
              result = jackoState->positionTransport(positionSeconds);
              jackoState->startTransport();
              if (result) {
                log(csound, "Failed to start Jack transport at %f seconds with"
                    " result: %d\n", positionSeconds, result);
              } else {
                log(csound, "Started Jack transport at %f seconds.\n",
                    positionSeconds);
              }
            }
            break;
         };
        }
      }
      return result;
  }
};

extern "C"
{
  static OENTRY oentries[] = {
    {
      (char *)"JackoInit",
      sizeof(JackoInit),
      0,
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackoInit::init_,
      0,
      0
    },
    {
      (char *)"JackoInfo",
      sizeof(JackoInfo),
      0,
      1,
      (char *)"",
      (char *)"",
      (SUBR)&JackoInfo::init_,
      0,
      0
    },
    {
      (char *)"JackoFreewheel",
      sizeof(JackoFreewheel),
      0,
      1,
      (char *)"",
      (char *)"i",
      (SUBR)&JackoFreewheel::init_,
      0,
      0
    },
    {
      (char *)"JackoOn",
      sizeof(JackoOn),
      0,
      1,
      (char *)"",
      (char *)"j",
      (SUBR)&JackoOn::init_,
      0,
      0
    },
    {
      (char *)"JackoAudioInConnect",
      sizeof(JackoAudioInConnect),
      0,
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackoAudioInConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackoAudioIn",
      sizeof(JackoAudioIn),
      0,
      5,
      (char *)"a",
      (char *)"S",
      (SUBR)&JackoAudioIn::init_,
      0,
      (SUBR)&JackoAudioIn::audio_,
    },
    {
      (char *)"JackoAudioOutConnect",
      sizeof(JackoAudioOutConnect),
      0,
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackoAudioOutConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackoAudioOut",
      sizeof(JackoAudioOut),
      0,
      5,
      (char *)"",
      (char *)"Sa",
      (SUBR)&JackoAudioOut::init_,
      0,
      (SUBR)&JackoAudioOut::audio_,
    },
    {
      (char *)"JackoMidiInConnect",
      sizeof(JackoMidiInConnect),
      0,
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackoMidiInConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackoMidiOutConnect",
      sizeof(JackoMidiOutConnect),
      0,
      1,
      (char *)"",
      (char *)"SS",
      (SUBR)&JackoMidiOutConnect::init_,
      0,
      0,
    },
    {
      (char *)"JackoMidiOut",
      sizeof(JackoMidiOut),
      0,
      3,
      (char *)"",
      (char *)"SkkkO",
      (SUBR)&JackoMidiOut::init_,
      (SUBR)&JackoMidiOut::kontrol_,
      0,
    },
    {
      (char *)"JackoNoteOut",
      sizeof(JackoNoteOut),
      0,
      3,
      (char *)"",
      (char *)"Siii",
      (SUBR)&JackoNoteOut::init_,
      (SUBR)&JackoNoteOut::kontrol_,
      0
    },
    {
      (char *)"JackoTransport",
      sizeof(JackoTransport),
      0,
      3,
      (char *)"",
      (char *)"kO", // O defaults to 0.
      (SUBR)&JackoTransport::init_,
      (SUBR)&JackoTransport::kontrol_,
      0
    },
    { 0, 0, 0, 0, 0, 0, (SUBR) 0, (SUBR) 0, (SUBR) 0 }
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
    int result = OK;
#pragma omp critical
    {
      std::map<CSOUND *, JackoState *>::iterator it =
        jackoStatesForCsoundInstances.find(csound);
      if (it != jackoStatesForCsoundInstances.end()) {
        //delete it->second;
        jackoStatesForCsoundInstances.erase(it);
      }
      //csound->Message(csound, "jacko: CsoundModuleDestroy(%p)\n", csound);
    }
    return result;
  }
}
