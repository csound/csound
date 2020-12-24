import { appendBuffers, decoder } from "@root/utils";
import { AUDIO_STATE, CALLBACK_DATA_BUFFER_SIZE, DATA_TYPE } from "@root/constants.js";

export const handleSABCallbacks = ({
  audioStatePointer,
  csound,
  callbackBuffer,
  callbackStringDataBuffer,
  combined,
}) => {
  const callbackBufferIndex = Atomics.load(audioStatePointer, AUDIO_STATE.CALLBACK_BUFFER_INDEX);
  if (callbackBufferIndex > 0) {
    let callbackIndex = 0;
    while (true) {
      if (callbackIndex >= callbackBufferIndex) {
        break;
      } else {
        const argumentCount = Atomics.load(callbackBuffer, callbackIndex);
        if (argumentCount == 0) {
          console.error("Csound SAB internal error, argumentcount is zero!");
        }

        // callbackIndex += 1;
        const argumentz = [];
        for (let argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++) {
          const argumentPosition = argumentIndex * 3 + 1 + callbackIndex;
          const dataType = Atomics.load(callbackBuffer, argumentPosition);
          const dataValue1 = Atomics.load(callbackBuffer, argumentPosition + 1);
          const dataValue2 = Atomics.load(callbackBuffer, argumentPosition + 2);
          if (dataType == DATA_TYPE.NUMBER) {
            argumentz.push(dataValue1);
          } else if (dataType == DATA_TYPE.STRING) {
            let dataString;
            if (dataValue1 > dataValue2) {
              const dataBuffer1 = new Uint8Array(CALLBACK_DATA_BUFFER_SIZE - dataValue1);
              dataBuffer1.set(callbackStringDataBuffer.subarray(dataValue1));
              const dataBuffer2 = new Uint8Array(dataValue2);
              dataBuffer2.set(callbackStringDataBuffer.subarray(0, dataValue2));
              dataString = decoder.decode(appendBuffers(dataBuffer1, dataBuffer2));
            } else {
              const dataBuffer = new Uint8Array(dataValue2 - dataValue1);
              dataBuffer.set(callbackStringDataBuffer.subarray(dataValue1, dataValue2));
              dataString = decoder.decode(dataBuffer);
            }
            argumentz.push(dataString);
          } else if (dataType == DATA_TYPE.FLOAT_ARRAY) {
            // TODO!!
          }
        }
        const [k, ...argz] = argumentz;
        const caller = combined.get(k);
        caller && caller.apply(undefined, [csound, ...argz]);
        callbackIndex += argumentCount - 1;
      }
    }
    Atomics.store(audioStatePointer, AUDIO_STATE.CALLBACK_BUFFER_INDEX, 0);
  }
};
