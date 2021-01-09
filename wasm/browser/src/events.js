import EE3 from "eventemitter3";

/**
 * @typedef PublicEvents
 * @readonly
 * @enum {number}
 * @property {string} "play" called anytime performance goes from pause/stop to a running state
 * @property {string} "pause" called after any successful csound.pause() calls
 * @property {string} "stop" called after end of performance or after a successful csound.stop()
 * @property {string} "realtimePerformanceStarted" called at the start of realtime performance but not on resume or render
 * @property {string} "realtimePerformancePaused" only called if csound.pause() was successfully called during performance
 * @property {string} "realtimePerformanceResumed" only called if csound.resume() was successfully called after a pause
 * @property {string} "realtimePerformanceEnded" called after end of performance or after a successful csound.stop()
 * @property {string} "renderStarted" called at the start of offline/non-realtime render to disk
 * @property {string} "renderEnded" called at the end of offline/non-realtime render to disk
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
    // this.eventEmitter.emit = this.eventEmitter.emit.bind(this.eventEmitter);
  }

  triggerRealtimePerformanceStarted() {
    this.eventEmitter.emit("realtimePerformanceStarted");
    if (this.currentDerivedPlayState == "pause" || this.currentDerivedPlayState == "stop") {
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

  decorateAPI(exportApi) {
    /**
     * Removes the specified listener from the listener array for the event named eventName.
     * @function
     * @name removeListener
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @param {function} listener
     * @return {external:EventEmitter}
     */
    exportApi["removeListener"] = this.eventEmitter.removeListener.bind(this.eventEmitter);
    /**
     * Returns an array listing the events for which the emitter has registered listeners.
     * The values in the array are strings.
     * @function
     * @name eventNames
     * @memberof CsoundObj
     * @return {Array<string>}
     */
    exportApi["eventNames"] = this.eventEmitter.eventNames.bind(this.eventEmitter);
    exportApi["listenerCount"] = this.eventEmitter.listenerCount.bind(this.eventEmitter);
    /**
     * Returns a copy of the array of listeners for the event named eventName.
     * @function
     * @name listeners
     * @memberof CsoundObj
     * @param {PublicEvents} eventName
     * @return {Array<function>}
     */
    exportApi["listeners"] = this.eventEmitter.listeners.bind(this.eventEmitter);
    exportApi["off"] = this.eventEmitter.off.bind(this.eventEmitter);
    exportApi["on"] = this.eventEmitter.on.bind(this.eventEmitter);
    exportApi["once"] = this.eventEmitter.once.bind(this.eventEmitter);
    exportApi["removeAllListeners"] = this.eventEmitter.removeAllListeners.bind(this.eventEmitter);
    exportApi["removeListener"] = this.eventEmitter.removeListener.bind(this.eventEmitter);
    return exportApi;
  }
}
