import { AUDIO_STATE, CALLBACK_DATA_BUFFER_SIZE, DATA_TYPE } from "@root/constants";
import { encoder } from "@root/utils";

let uid = 0;

export const makeSABPerfCallback = ({
  apiK,
  audioStatePointer,
  callbackBuffer,
  callbackStringDataBuffer,
  callbackFloatArrayDataBuffer,
  returnQueue,
}) => (args) => {
  uid += 1;
  let rejectCallback;
  const returnPromise = new Promise((resolve, reject) => {
    returnQueue[uid] = resolve;
    rejectCallback = reject;
  });
  const argumentz = [apiK, ...args];
  const callbackBufferIndex = Atomics.load(audioStatePointer, AUDIO_STATE.CALLBACK_BUFFER_INDEX);
  Atomics.store(callbackBuffer, callbackBufferIndex, argumentz.length);
  Atomics.store(callbackBuffer, callbackBufferIndex + 1, argumentz.length);
  argumentz.forEach((argument, index) => {
    const argumentPosition = callbackBufferIndex + index * 3 + 2;
    if (typeof argument === "number") {
      const dataType = DATA_TYPE.NUMBER;
      Atomics.store(callbackBuffer, argumentPosition, dataType);
      Atomics.store(callbackBuffer, argumentPosition + 1, argument);
    } else if (typeof argument == "string") {
      const callbackStringDataBufferIndex = Atomics.load(
        audioStatePointer,
        AUDIO_STATE.CALLBACK_STRING_DATA_BUFFER_POS,
      );
      const dataType = DATA_TYPE.STRING;
      Atomics.store(callbackBuffer, argumentPosition, dataType);
      if (callbackStringDataBufferIndex + argument.length >= CALLBACK_DATA_BUFFER_SIZE) {
        const slice1Len = CALLBACK_DATA_BUFFER_SIZE - callbackStringDataBufferIndex;
        const stringBuffer1 = encoder.encode(argument.substring(0, slice1Len));
        const stringBuffer2 = encoder.encode(argument.substring(slice1Len));
        Atomics.store(audioStatePointer, AUDIO_STATE.CALLBACK_STRING_DATA_BUFFER_POS, slice1Len);
        callbackStringDataBuffer.set(stringBuffer1, callbackStringDataBufferIndex);
        callbackStringDataBuffer.set(stringBuffer2, 0);
        Atomics.store(callbackBuffer, argumentPosition + 1, callbackStringDataBufferIndex);
        Atomics.store(callbackBuffer, argumentPosition + 2, argument.length - slice1Len);
      } else {
        const stringBuffer = encoder.encode(argument);
        callbackStringDataBuffer.set(stringBuffer, callbackStringDataBufferIndex);
        Atomics.store(
          audioStatePointer,
          AUDIO_STATE.CALLBACK_STRING_DATA_BUFFER_POS,
          callbackStringDataBufferIndex + argument.length,
        );
        Atomics.store(callbackBuffer, argumentPosition + 1, callbackStringDataBufferIndex);
        Atomics.store(
          callbackBuffer,
          argumentPosition + 2,
          callbackStringDataBufferIndex + argument.length,
        );
      }
    } else if (
      typeof argument == "object" &&
      (argument.constructor === Float32Array || argument.constructor === Float64Array)
    ) {
      console.error(`TODO!`);
    } else {
      console.error(`Illegal argument in ${apiK}: ${argument}`);
      rejectCallback(`Illegal argument ${argument}`);
    }
  });
  Atomics.add(audioStatePointer, AUDIO_STATE.CALLBACK_BUFFER_INDEX, callbackBufferIndex + 1);
  return returnPromise;
};
