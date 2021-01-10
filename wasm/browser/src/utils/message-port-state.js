export function MessagePortState() {}
MessagePortState.prototype.ready = 0;
MessagePortState.prototype.port = undefined;
MessagePortState.prototype.post = () => {};
MessagePortState.prototype.broadcastPlayState = () => {};
MessagePortState.prototype.workerState = undefined;
MessagePortState.prototype.vanillaWorkerState = undefined;

export default MessagePortState;
