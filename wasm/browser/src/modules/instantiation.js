/*
   csound instantiation module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * creates Csound instance
 * (inferred in @csound/wasm/browser)
 */
export const csoundCreate = (wasm) => () => wasm["exports"]["csoundCreateWasi"]();

csoundCreate.toString = () => "create = async () => undefined;";

/**
 * Destroys an instance of Csound and frees memory
 * @function
 */
export const csoundDestroy = (wasm) => (csound) => wasm["exports"]["csoundDestroy"](csound);

csoundDestroy.toString = () => "destroy = async () => undefined;";

/**
 * Returns the API version as int
 * @function
 */
export const csoundGetAPIVersion = (wasm) => () => wasm["exports"]["csoundGetAPIVersion"]();

csoundGetAPIVersion.toString = () => "getAPIVersion = async () => Number;";

/**
 * Returns the Csound version as int
 * @function
 */
export const csoundGetVersion = (wasm) => () => wasm["exports"]["csoundGetVersion"]();

csoundGetVersion.toString = () => "getVersion = async () => Number;";

/**
 * Initialise Csound with specific flags.
 * This function is called internally by csoundCreate(),
 * so there is generally no need to use it explicitly
 * unless you need to avoid default initilization that
 * sets signal handlers and atexit() callbacks.
 * @function
 */
export const csoundInitialize = (wasm) => (_, flags) => wasm["exports"]["csoundInitialize"](flags);

csoundInitialize.toString = () => "initialize = async () => Number;";
