/**
 * @constructor
 */
export function MessagePortState() {}

/**
 * @type {boolean}
 */
MessagePortState.prototype.ready = false;

/**
 * @type {Function|undefined}
 */
MessagePortState.prototype.port = undefined;

/**
 * @type {Function}
 */
MessagePortState.prototype.post = () => {};
/**
 * @type {Function}
 */
MessagePortState.prototype.broadcastPlayState = () => {};
/**
 * @type {string|undefined}
 */
MessagePortState.prototype.workerState = undefined;
/**
 * @type {string|undefined}
 */
MessagePortState.prototype.vanillaWorkerState = undefined;

export default MessagePortState;
