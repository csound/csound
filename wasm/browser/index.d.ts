import EventEmitter from "eventemitter3";

declare interface CsoundFs {
  appendFile: (path: string, file: Uint8Array) => Promise<void>;
  writeFile: (path: string, file: Uint8Array) => Promise<void>;
  readFile: (path: string) => Promise<Uint8Array>;
  unlink: (path: string) => Promise<void>;
  readdir: (path: string) => Promise<string[]>;
  mkdir: (path: string) => Promise<void>;
}

/**
 * @property "play" - called anytime performance goes from pause/stop to a running state.
 * @property "pause" - called after any successful csound.pause() calls.
 * @property "stop" - called after end of performance or after a successful csound.stop().
 * @property "realtimePerformanceStarted" - called at the start of realtime performance but not on resume or render.
 * @property "realtimePerformancePaused" - only called if csound.pause() was successfully called during performance.
 * @property "realtimePerformanceResumed" - only called if csound.resume() was successfully called after a pause.
 * @property "realtimePerformanceEnded" - called after end of performance or after a successful csound.stop().
 * @property "renderStarted" - called at the start of offline/non-realtime render to disk.
 * @property "renderEnded" - called at the end of offline/non-realtime render to disk.
 * @property "onAudioNodeCreated" - called when an audioNode is created from the AudioContext before realtime performance.
 * the event callback will include the audioNode itself, which is needed if autoConnect is set to false.
 * @property "message" - the main entrypoint to csound's messaging (-m) system,
 * a default event listener will print the message to the browser console, this default
 * listener can be removed by the user.
 */
declare type PublicEvents =
  | "play"
  | "pause"
  | "stop"
  | "realtimePerformanceStarted"
  | "realtimePerformancePaused"
  | "realtimePerformanceResumed"
  | "realtimePerformanceEnded"
  | "renderStarted"
  | "renderEnded"
  | "onAudioNodeCreated"
  | "message";

/**
 * CsoundObj API.
 */
declare interface CsoundObj {
  /** Returns the AudioContext used to create AudioNodes. May be one passed as initialization parameters to
   * Csound() or one created by Csound itself if none are provided.
   */
  getAudioContext: () => Promise<AudioContext | undefined>;

  /** Returns the AudioNode used with Csound processing. May return undefined if node has not yet been created.
   * Single-thread (the default) backends will have nodes available right after initializing Csound, while worker-backed
   * versions of Csound may construct the node later when Csound starts.
   */
  getNode: () => Promise<AudioNode | undefined>;

  /**
   * Returns an array listing the events for which the emitter has registered listeners.
   * The values in the array are strings.
   */
  eventNames: () => string[];
  /**
   * Returns the number of listeners listening to the event named eventName.
   */
  listenerCount: () => number;
  /**
   * Returns a copy of the array of listeners for the event named eventName.
   */
  listeners: (eventName: PublicEvents) => Array<EventEmitter.EventListener<PublicEvents, any>>;
  /**
   * Alias for removeListener()
   */
  off: (
    eventName: PublicEvents,
    listener: EventEmitter.EventListener<PublicEvents, any>,
  ) => EventEmitter;
  /**
   * Adds the listener to the end of the listeners array for the event named eventName.
   * No checks are made to see if the listener has already been added.
   * Multiple calls passing the same combination of eventName and listener
   * will result in the listener being added, and called, multiple times.
   */
  on: (
    eventName: PublicEvents,
    listener: EventEmitter.EventListener<PublicEvents, any>,
  ) => EventEmitter;
  /**
   * Alias for "on"
   */
  addListener: (
    eventName: PublicEvents,
    listener: EventEmitter.EventListener<PublicEvents, any>,
  ) => EventEmitter;
  /**
   * Adds a one-time listener for the event named eventName.
   * The next time eventName is triggered, this listener is removed and then invoked.
   */
  once: (
    eventName: PublicEvents,
    listener: EventEmitter.EventListener<PublicEvents, any>,
  ) => EventEmitter;
  /**
   * Removes all listeners, or those of the specified eventName.
   * It is bad practice to remove listeners added elsewhere in the code,
   * particularly when the EventEmitter instance was created by some other
   * component or module.
   * Returns a reference to the EventEmitter, so that calls can be chained.
   */
  removeAllListeners: (eventName: PublicEvents) => EventEmitter;
  /**
   * Removes the specified listener from the listener array for the event named eventName.
   */
  removeListener: (
    eventName: PublicEvents,
    listener: EventEmitter.EventListener<PublicEvents, any>,
  ) => EventEmitter;
  /**
   * CsoundFilesystem
   */
  fs: CsoundFs;
  /**
   * Returns the sample rate from Csound instance
   */
  getSr: () => Promise<number>;
  /**
   * Returns the control rate from Csound instance
   */
  getKr: () => Promise<number>;
  /**
   * Returns the ksmps value (kr/sr) from Csound instance
   */
  getKsmps: () => Promise<number>;
  /**
   * Returns the number of output channels from Csound instance
   */
  getNchnls: () => Promise<number>;
  /**
   * Returns the number of input channels from Csound instance
   */
  getNchnlsInput: () => Promise<number>;
  /**
   * Returns the value of csoundGet0dBFS
   */
  get0dBFS: () => Promise<number>;
  /**
   * Returns the A4 frequency reference
   */
  getA4: () => Promise<number>;
  /**
   * Return the current performance time in samples
   */
  getCurrentTimeSamples: () => Promise<number>;
  /**
   * Return the size of MYFLT in number of bytes
   */
  getSizeOfMYFLT: () => Promise<number>;
  /**
   * Set a single csound option (flag),
   * no spaces are allowed in the string.
   */
  setOption: (option: string) => Promise<number>;
  /**
   * Configure Csound with a given set of
   * parameters defined in the CSOUND_PARAMS structure.
   * These parameters are the part of the OPARMS struct
   * that are configurable through command line flags.
   * The CSOUND_PARAMS structure can be obtained using
   * csoundGetParams().
   * These options should only be changed before
   * performance has started.
   * @param csoundParams - csoundParams object
   */
  setParams: (csoundParams: CSOUND_PARAMS) => Promise<undefined>;
  /**
   * Get the current set of parameters
   * from a Csound instance
   * in a CSOUND_PARAMS structure.
   * @returns - CSOUND_PARAMS object
   */
  getParams: () => Promise<CSOUND_PARAMS>;
  /**
   * Returns whether Csound is set to print debug messages
   * sent through the DebugMsg() internal API function.
   * Anything different to 0 means true.
   */
  getDebug: () => Promise<number>;
  /**
   * Return the size of MYFLT in number of bytes
   */
  setDebug: (debug: number) => Promise<undefined>;
  /**
   * Inputs an immediate score event
   * without any pre-process parsing
   */
  inputMessage: (scoreEvent: string) => Promise<number>;
  /**
   * Inputs an immediate score event
   * without any pre-process parsing
   */
  inputMessageAsync: (scoreEvent: string) => Promise<number>;
  /**
   * Retrieves the value of control channel identified by channelName.
   * If the err argument is not NULL, the error (or success) code finding
   * or accessing the channel is stored in it.
   */
  getControlChannel: (channelName: string) => Promise<undefined>;
  /**
   * Sets the value of control channel identified by channelName
   */
  setControlChannel: (channelName: string, value: number) => Promise<undefined>;
  /**
   * Retrieves the string channel identified by channelName
   */
  getStringChannel: (channelName: string) => Promise<undefined>;
  /**
   * Sets the string channel value identified by channelName
   */
  setStringChannel: (channelName: string, value: string) => Promise<undefined>;
  /**
   * Returns the audio output name (-o)
   */
  getOutputName: () => Promise<string>;
  /**
   * Returns the audio input name (-i)
   */
  getInputName: () => Promise<string>;
  /**
   * Destroys an instance of Csound and frees memory
   */
  destroy: () => Promise<undefined>;
  /**
   * Returns the API version as int
   */
  getAPIVersion: () => Promise<number>;
  /**
   * Returns the Csound version as int
   */
  getVersion: () => Promise<number>;
  /**
   * Initialise Csound with specific flags.
   * This is called internally by csoundCreate(),
   * so there is generally no need to use it explicitly
   * unless you need to avoid default initilization that
   * sets signal handlers and atexit() callbacks.
   * @returns - Return value is zero on success,
   *     positive if initialisation was done already, and negative on error.
   */
  initialize: () => Promise<number>;
  /**
   * Parses a csound orchestra string
   */
  parseOrc: (orc: string) => Promise<object>;
  /**
   * Compiles AST tree
   */
  compileTree: (tree: any) => Promise<number>;
  /**
   * Compiles a csound orchestra string
   */
  compileOrc: (orc: string) => Promise<number>;
  /**
   * Compiles a csound orchestra string
   */
  evalCode: (orc: string) => Promise<number>;
  /**
   * Prepares Csound for performance
   */
  start: () => Promise<number>;
  /**
   * Compiles a Csound input file but does not perform it.
   */
  compileCsd: (path: string) => Promise<number>;
  /**
   * Compiles a CSD string but does not perform it.
   */
  compileCsdText: (orc: string) => Promise<number>;
  /**
   * Pauses a performance if it's running
   */
  pause: () => Promise<undefined>;
  /**
   * Resumes a performance if it's paused
   */
  resume: () => Promise<undefined>;
  /**
   * Performs(plays) audio until end is reached
   */
  perform: () => Promise<number>;
  /**
   * Performs(plays) 1 ksmps worth of sample(s)
   */
  performKsmps: () => Promise<number>;
  /**
   * Performs(plays) 1 buffer worth of audio
   */
  performBuffer: () => Promise<number>;
  /**
   * Stops a csoundPerform
   */
  stop: () => Promise<undefined>;
  /**
   * Prints information about the end of a performance,
   * and closes audio and MIDI devices.
   */
  cleanup: () => Promise<number>;
  /**
   * Prints information about the end of a performance,
   * and closes audio and MIDI devices.
   */
  reset: () => Promise<number>;
  /**
   * Returns the number of samples in Csound's input buffer.
   */
  getInputBufferSize: () => Promise<number>;
  /**
   * Returns the number of samples in Csound's output buffer.
   */
  getOutputBufferSize: () => Promise<number>;
  /**
   * Returns the address of the Csound audio input buffer.
   */
  getInputBuffer: () => Promise<number>;
  /**
   * Returns the address of the Csound audio output buffer.
   */
  getOutputBuffer: () => Promise<number>;
  /**
   * Returns the address of the Csound audio input working buffer (spin).
   * Enables external software to write audio into Csound before calling csoundPerformKsmps.
   */
  getSpin: () => Promise<number>;
  /**
   * Returns the address of the Csound audio output working buffer (spout).
   * Enables external software to read audio from Csound after calling csoundPerformKsmps.
   */
  getSpout: () => Promise<number>;
  /**
   * Sees whether Csound score events are performed or not,
   * independently of real-time MIDI events
   */
  isScorePending: () => Promise<number>;
  /**
   * Sets whether Csound score events are performed or not
   * (real-time events will continue to be performed).
   * Can be used by external software, such as a VST host,
   * to turn off performance of score events (while continuing to perform real-time events),
   * for example to mute a Csound score while working on other tracks of a piece,
   * or to play the Csound instruments live.
   */
  setScorePending: (pending: number) => Promise<undefined>;
  /**
   * Read, preprocess, and load a score from an ASCII string It can be called repeatedly,
   * with the new score events being added to the currently scheduled ones.
   */
  readScore: (score: string) => Promise<undefined>;
  /**
   * Returns the current score time in seconds since the beginning of performance.
   */
  getScoreTime: () => Promise<number>;
  /**
   * Returns the score time beginning at which score events will actually immediately be performed
   */
  getScoreOffsetSeconds: () => Promise<number>;
  /**
   * Csound score events prior to the specified time are not performed,
   * and performance begins immediately at the specified time
   * (real-time events will continue to be performed as they are received).
   * Can be used by external software, such as a VST host, to begin
   * score performance midway through a Csound score,
   * for example to repeat a loop in a sequencer,
   * or to synchronize other events with the Csound score.
   */
  setScoreOffsetSeconds: (time: number) => Promise<number>;
  /**
   * Rewinds a compiled Csound score to the time specified with csoundObj.setScoreOffsetSeconds().
   */
  rewindScore: () => Promise<number>;
  /**
   * Returns the length of a table
   * (not including the guard point),
   * or -1 if the table does not exist.
   */
  tableLength: (tableNum: string) => Promise<number>;
  /**
   * Returns the value of a slot in a table.
   * The table number and index are assumed to be valid.
   */
  tableGet: (tableNum: string, tableIndex: string) => Promise<number>;
  /**
   * Sets the value of a slot in a table.
   * The table number and index are assumed to be valid.
   */
  tableSet: (tableNum: string, tableIndex: string, value: string) => Promise<undefined>;
  /**
   * Copy the contents of an Array or TypedArray from javascript into a given csound table.
   * The table number is assumed to be valid, and the table needs to have sufficient space
   * to receive all the array contents.
   */
  tableCopyIn: (tableNum: string, array: number[] | ArrayLike<number>) => Promise<undefined>;
  /**
   * Copies the contents of a table from csound into Float64Array.
   * The returns a Float64Array if the table exists, otherwise
   * it returns undefined.
   */
  tableCopyOut: (tableNum: string) => Promise<Float64Array | undefined>;
  getTable: (tableNum: string) => Promise<Float64Array | undefined>;
  /**
   * Copies the contents of a table from csound into Float64Array.
   * The returns a Float64Array if the table exists, otherwise
   * it returns undefined.
   */
  getTableArgs: (tableNum: string) => Promise<Float64Array | undefined>;
  /**
   * Checks if a given GEN number num is a named GEN if so,
   * it returns the string length (excluding terminating NULL char).
   * Otherwise it returns 0.
   */
  isNamedGEN: (tableNum: string) => Promise<number>;
  /**
   * Gets the GEN name from a number num, if this is a named GEN.
   * If the table number doesn't represent a named GEN, it will
   * return undefined.
   */
  getNamedGEN: (tableNum: string) => Promise<string | undefined>;
  /**
   * Emit a midi message
   */
  midiMessage: (status: number, data1: number, data2: number) => Promise<undefined>;
  /**
   * Terminates an instances and all its workers, making disabling any futher uses of a given instance.
   */
  terminateInstance: () => Promise<void>;
}

/**
 * The default entry for @csound/wasm/browser module.
 * If loaded successfully, it returns CsoundObj,
 * otherwise undefined.
 * @param [params] - Initialization parameters
 * @param [params.audioContext] - Optional AudioContext to use; if none given, an AudioContext will be created.
 * @param [params.inputChannelCount] - Optional input channel count for AudioNode used with WebAudio graph.
 * Defaults to the value of nchnls_i in useWorker but 2 otherwise.
 * @param [params.outputChannelCount] - Optional output channel count AudioNode used with WebAudio graph.
 * Defaults to the value of nchnls in useWorker but 2 otherwise.
 * @param [params.autoConnect = true] - Set to configure Csound to automatically connect to the audioContext.destination output.
 * @param [params.withPlugins] - Array of WebAssembly Csound plugin libraries to use with Csound.
 * @param [params.useWorker = false] - Configure to use backend using Web Workers to run Csound in a thread separate from audio callback.
 * @param [params.useSAB = true] - Configure to use SharedArrayBuffers for WebWorker communications if platform supports it.
 * @param [params.useSPN = false] - Configure to use explicitly request ScriptProcessorNode rather than AudioWorklet. Recommended only for debug testing purposes.
 */
declare function Csound(params?: {
  audioContext?: AudioContext;
  inputChannelCount?: number;
  outputChannelCount?: number;
  autoConnect?: boolean;
  withPlugins?: object[];
  useWorker?: boolean;
  useSAB?: boolean;
  useSPN?: boolean;
}): Promise<CsoundObj | undefined>;

declare type Csound = typeof Csound;

declare type CSOUND_PARAMS = {
  debug_mode: number;
  buffer_frames: number;
  hardware_buffer_frames: number;
  displays: number;
  ascii_graphs: number;
  postscript_graphs: number;
  message_level: number;
  tempo: number;
  ring_bell: number;
  use_cscore: number;
  terminate_on_midi: number;
  heartbeat: number;
  defer_gen01_load: number;
  midi_key: number;
  midi_key_cps: number;
  midi_key_oct: number;
  midi_key_pch: number;
  midi_velocity: number;
};
