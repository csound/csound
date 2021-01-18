export const DEFAULT_HARDWARE_BUFFER_SIZE = 4096;
export const DEFAULT_SOFTWARE_BUFFER_SIZE = 512;
export const MAX_CHANNELS = 32;
export const MAX_HARDWARE_BUFFER_SIZE = 16384;
export const MIDI_BUFFER_SIZE = 1024;
export const MIDI_BUFFER_PAYLOAD_SIZE = 3;
export const CALLBACK_DATA_BUFFER_SIZE = 16384;
// export const CALLBACK_DATA_BUFFER_SIZE = 1024;

export const initialSharedState = [
  0, // 1 = Worklet requests new buffer data (atomic notify)
  0, // 1 = Csound is currently performing
  0, // 1 = Csound is currently paused
  0, // 1 = STOP
  0, // n = performKsmps call count
  2, // n = nchnls
  0, // n = ncnls_i
  DEFAULT_HARDWARE_BUFFER_SIZE, // n = [hardware -B] bufferSize
  DEFAULT_SOFTWARE_BUFFER_SIZE, // n = [software -b] bufferSize
  0, // n = number of input buffers available
  0, // n = number of output buffers available
  0, // n = buffer read index of input buffer
  0, // n = buffer read index of output buffer
  0, // n = buffer write index of input buffer
  0, // n = buffer write index of output buffer
  44100, // sample rate
  0, // n = if 1 then is requesting rtmidi
  0, // n = rtmidi buffer index
  0, // n = available rtmidi events in buffer
  0, // n = has pending callbacks
];

// Enum helper for SAB
export const AUDIO_STATE = {
  ATOMIC_NOTIFY: 0,
  IS_PERFORMING: 1,
  IS_PAUSED: 2,
  STOP: 3,
  PERF_LOOP_CNT: 4,
  NCHNLS: 5,
  NCHNLS_I: 6,
  HW_BUFFER_SIZE: 7,
  SW_BUFFER_SIZE: 8,
  AVAIL_IN_BUFS: 9,
  AVAIL_OUT_BUFS: 10,
  INPUT_READ_INDEX: 11,
  OUTPUT_READ_INDEX: 12,
  INPUT_WRITE_INDEX: 13,
  OUTPUT_WRITE_INDEX: 14,
  SAMPLE_RATE: 15,
  IS_REQUESTING_RTMIDI: 16,
  RTMIDI_INDEX: 17,
  AVAIL_RTMIDI_EVENTS: 18,
  HAS_PENDING_CALLBACKS: 19,
};

export const DATA_TYPE = {
  NUMBER: 0,
  STRING: 1,
  FLOAT_32: 2,
  FLOAT_64: 3,
};
