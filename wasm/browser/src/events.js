import EE3 from "eventemitter3";

const EventEmitter = new EE3();

export const decorateAPI = (exportApi) => {
  exportApi["newListener"] = EventEmitter.newListener;
  exportApi["removeListener"] = EventEmitter.removeListener;
  exportApi["eventNames"] = EventEmitter.eventNames;
  exportApi["getMaxListeners"] = EventEmitter.getMaxListeners;
  exportApi["listenerCount"] = EventEmitter.listenerCount;
  exportApi["listeners"] = EventEmitter.listeners;
  exportApi["off"] = EventEmitter.off;
  exportApi["on"] = EventEmitter.on;
  exportApi["once"] = EventEmitter.once;
  exportApi["prependListener"] = EventEmitter.prependListener;
  exportApi["prependOnceListener"] = EventEmitter.prependOnceListener;
  exportApi["removeAllListeners"] = EventEmitter.removeAllListeners;
  exportApi["removeListener"] = EventEmitter.removeListener;
  exportApi["setMaxListeners"] = EventEmitter.setMaxListeners;
  exportApi["rawListeners"] = EventEmitter.rawListeners;
  return exportApi;
};

// const EVENTS = [
//   // Internal events
//   "realtimePerformanceStarted",
//   "realtimePerformancePaused",
//   "realtimePerformanceResumed",
//   "realtimePerformanceEnded",
//   "renderStarted",
//   "renderEnded",
// ];

// State AND Event names of derived
// high-level state
// const STATES = ["play", "pause", "stop"];

export const triggerRealtimePerformanceStarted = (that) => {
  EventEmitter.emit("realtimePerformanceStarted");
  if (that.currentDerivedPlayState == "pause" || that.currentDerivedPlayState == "stop") {
    EventEmitter.emit("play");
    that.currentDerivedPlayState = "play";
  }
};

export const triggerRealtimePerformancePaused = (that) => {
  EventEmitter.emit("realtimePerformancePaused");
  if (that.currentDerivedPlayState !== "pause") {
    EventEmitter.emit("pause");
    that.currentDerivedPlayState = "pause";
  }
};

export const triggerRealtimePerformanceResumed = (that) => {
  EventEmitter.emit("realtimePerformanceResumed");
  if (that.currentDerivedPlayState !== "play") {
    EventEmitter.emit("play");
    that.currentDerivedPlayState = "play";
  }
};

export const triggerRealtimePerformanceEnded = (that) => {
  new CustomEvent("realtimePerformanceEnded", {
    cancelable: true,
  });
  if (that.currentDerivedPlayState !== "stop") {
    EventEmitter.emit("stop");
    that.currentDerivedPlayState = "stop";
  }
};

export const triggerRenderStarted = (that) => {
  EventEmitter.emit("renderStarted");
  if (that.currentDerivedPlayState !== "stop") {
    EventEmitter.emit("stop");
    that.currentDerivedPlayState = "stop";
  }
};

export const triggerRenderEnded = (that) => {
  EventEmitter.emit("renderEnded");
  if (that.currentDerivedPlayState !== "stop") {
    EventEmitter.emit("stop");
    that.currentDerivedPlayState = "stop";
  }
};
