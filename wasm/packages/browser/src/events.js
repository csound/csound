import EE3 from "eventemitter3/umd/eventemitter3.min.js";

/**
 * @template PublicEvents
 * @readonly
 * @enum {number}
 * @property {string} "play" called anytime performance goes from pause/stop to a running state.
 * @property {string} "pause" called after any successful csound.pause() calls.
 * @property {string} "stop" called after end of performance or after a successful csound.stop().
 * @property {string} "realtimePerformanceStarted" called at the start of realtime performance but not on resume or render.
 * @property {string} "realtimePerformancePaused" only called if csound.pause() was successfully called during performance.
 * @property {string} "realtimePerformanceResumed" only called if csound.resume() was successfully called after a pause.
 * @property {string} "realtimePerformanceEnded" called after end of performance or after a successful csound.stop().
 * @property {string} "renderStarted" called at the start of offline/non-realtime render to disk.
 * @property {string} "renderEnded" called at the end of offline/non-realtime render to disk.
 * @property {string} "onAudioNodeCreated" called when an audioNode is created from the AudioContext before realtime performance.
 * the event callback will include the audioNode itself, which is needed if autoConnect is set to false.
 * @property {string} "message" the main entrypoint to csound's messaging (-m) system,
 * a default event listener will print the message to the browser console, this default
 * listener can be removed by the user.
 * @suppress {misplacedTypeAnnotation}
 */

export class PublicEventAPI {
  constructor(parent) {
    this.parent = parent;
    this.eventEmitter = new EE3();
    this.currentDerivedPlayState = undefined;
    this.decorateAPI = this.decorateAPI.bind(this);
    this.triggerRealtimePerformanceStarted = this.triggerRealtimePerformanceStarted.bind(this);
    this.triggerRealtimePerformancePaused = this.triggerRealtimePerformancePaused.bind(this);
    this.triggerRealtimePerformanceResumed = this.triggerRealtimePerformanceResumed.bind(this);
    this.triggerRealtimePerformanceEnded = this.triggerRealtimePerformanceEnded.bind(this);
    this.triggerRenderStarted = this.triggerRenderStarted.bind(this);
    this.triggerRenderEnded = this.triggerRenderEnded.bind(this);
    this.triggerOnAudioNodeCreated = this.triggerOnAudioNodeCreated.bind(this);
    this.terminateInstance = this.terminateInstance.bind(this);
  }

  terminateInstance() {
    this.eventEmitter.removeAllListeners([
      "play",
      "pause",
      "stop",
      "realtimePerformanceStarted",
      "realtimePerformancePaused",
      "realtimePerformanceResumed",
      "realtimePerformanceEnded",
      "renderStarted",
      "renderEnded",
      "onAudioNodeCreated",
      "message",
    ]);
    Object.keys(this).forEach((key) => delete this[key]);
  }

  triggerRealtimePerformanceStarted() {
    this.eventEmitter.emit("realtimePerformanceStarted");
    if (this.currentDerivedPlayState !== "play") {
      this.eventEmitter.emit("play");
      this.currentDerivedPlayState = "play";
    }
  }

  triggerRealtimePerformancePaused() {
    this.eventEmitter.emit("realtimePerformancePaused");
    if (this.currentDerivedPlayState !== "pause") {
      this.eventEmitter.emit("pause");
      this.currentDerivedPlayState = "pause";
    }
  }

  triggerRealtimePerformanceResumed() {
    this.eventEmitter.emit("realtimePerformanceResumed");
    if (this.currentDerivedPlayState !== "play") {
      this.eventEmitter.emit("play");
      this.currentDerivedPlayState = "play";
    }
  }

  triggerRealtimePerformanceEnded() {
    this.eventEmitter.emit("realtimePerformanceEnded");
    if (this.currentDerivedPlayState !== "stop") {
      this.eventEmitter.emit("stop");
      this.currentDerivedPlayState = "stop";
    }
  }

  triggerRenderStarted() {
    this.eventEmitter.emit("renderStarted");
    if (this.currentDerivedPlayState !== "stop") {
      this.eventEmitter.emit("stop");
      this.currentDerivedPlayState = "stop";
    }
  }

  triggerRenderEnded() {
    this.eventEmitter.emit("renderEnded");
    if (this.currentDerivedPlayState !== "stop") {
      this.eventEmitter.emit("stop");
      this.currentDerivedPlayState = "stop";
    }
  }

  triggerOnAudioNodeCreated(audioNode) {
    this.eventEmitter.emit("onAudioNodeCreated", audioNode);
  }

  triggerMessage({ log }) {
    this.eventEmitter.emit("message", log);
  }

  decorateAPI(exportApi) {
    /**
     * Returns an array listing the events for which the emitter has registered listeners.
     * The values in the array are strings.
     * @function
     * @name eventNames
     * @memberof CsoundObj
     * @return {Array<string>}
     */
    exportApi.eventNames = this.eventEmitter.eventNames.bind(this.eventEmitter);
    /**
     * Returns the number of listeners listening to the event named eventName.
     * @function
     * @name listenerCount
     * @memberof CsoundObj
     * @return {number}
     */
    exportApi.listenerCount = this.eventEmitter.listenerCount.bind(this.eventEmitter);
    /**
     * Returns a copy of the array of listeners for the event named eventName.
     * @function
     * @name listeners
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @return {Array.<function()>}
     */
    exportApi.listeners = this.eventEmitter.listeners.bind(this.eventEmitter);
    /**
     * Alias for removeListener()
     * @function
     * @name off
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function()} listener
     * @return {EventEmitter}
     */
    exportApi.off = this.eventEmitter.off.bind(this.eventEmitter);
    /**
     * Adds the listener function to the end of the listeners array for the event named eventName.
     * No checks are made to see if the listener has already been added.
     * Multiple calls passing the same combination of eventName and listener
     * will result in the listener being added, and called, multiple times.
     * @function
     * @name on
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function()} listener
     * @return {EventEmitter}
     */
    exportApi.on = this.eventEmitter.on.bind(this.eventEmitter);
    /**
     * Alias for "on"
     * @function
     * @name addListener
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function()} listener
     * @return {EventEmitter}
     */
    exportApi.addListener = this.eventEmitter.on.bind(this.eventEmitter);
    /**
     * Adds a one-time listener function for the event named eventName.
     * The next time eventName is triggered, this listener is removed and then invoked.
     * @function
     * @name once
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function()} listener
     * @return {EventEmitter}
     */
    exportApi.once = this.eventEmitter.once.bind(this.eventEmitter);
    /**
     * Removes all listeners, or those of the specified eventName.
     * It is bad practice to remove listeners added elsewhere in the code,
     * particularly when the EventEmitter instance was created by some other
     * component or module.
     * Returns a reference to the EventEmitter, so that calls can be chained.
     * @function
     * @name removeAllListeners
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @return {EventEmitter}
     */
    exportApi.removeAllListeners = this.eventEmitter.removeAllListeners.bind(this.eventEmitter);
    /**
     * Removes the specified listener from the listener array for the event named eventName.
     * removeListener() will remove, at most, one instance of a listener from the listener array.
     * If any single listener has been added multiple times to the listener array for the specified eventName,
     * then removeListener() must be called multiple times to remove each instance.
     * Removes the specified listener from the listener array for the event named eventName.
     * @function
     * @name removeListener
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function()} listener
     * @return {EventEmitter}
     */
    exportApi.removeListener = this.eventEmitter.removeListener.bind(this.eventEmitter);
    return exportApi;
  }
}
