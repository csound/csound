# Table convolution outputs

`ftconv` and `dconv` accept 1–32 separate audio outputs. Both also have an
audio-array form with an explicit channel count and no fixed channel limit.
Available memory and Csound's array and table sizes still bound that count.

```csound
aLeft, aRight ftconv aInput, iIR, iPartLen [, iSkip, iLength, iSkipInit]
aLeft, aRight dconv aInput, iLength, iIR

aOut[] ftconv aInput, iIR, iPartLen, iChannels [, iSkip, iLength, iSkipInit]
aOut[] dconv aInput, iLength, iIR, iChannels
```

The array forms allocate or resize `aOut` at init. `iChannels` must be a positive
whole number. Each array element holds one audio channel. For example:

```csound
; iIR holds a 64-channel interleaved impulse response.
aWet[] ftconv aInput, iIR, 128, 64
out aWet
```

Both opcodes convolve one input signal with each channel of the table. Store
the impulse responses in interleaved order: all channels of frame 0, then all
channels of frame 1, and so on. The number of separate outputs, or `iChannels`
for an array, sets how the opcode reads that order. The table's file-channel
metadata does not set the output count. Incomplete trailing frames are ignored.

Lengths and skip offsets count **frames per channel**, not individual table
entries. For `dconv`, `iLength` limits the number of frames to use and cannot
exceed the complete frames in the table. It keeps its existing truncation of
fractional lengths. For `ftconv`, the existing partition, skip, length, and
skip-init rules still apply. A change in the channel count resets its state,
even when `iSkipInit` is nonzero.

`dconv` adds no delay. `ftconv` delays the result by `iPartLen` samples. Existing
mono `dconv` calls and existing `ftconv` calls keep their meaning. Avoid shrinking
the output array during performance; reinit the opcode to change its channel count.
