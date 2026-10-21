import { RING_BUFFER_SIZE } from "../constants.js";

// Typed-array offsets are bytes; the shared ring capacity counts samples.
export const sharedAudioChannels = (buffer, count) =>
  Array.from(
    { length: count },
    (_, index) =>
      new Float64Array(
        buffer,
        index * RING_BUFFER_SIZE * Float64Array.BYTES_PER_ELEMENT,
        RING_BUFFER_SIZE,
      ),
  );
