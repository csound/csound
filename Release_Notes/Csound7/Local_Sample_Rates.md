# Local sample rates

[Csound 7 release notes](../Version_7.00.md)

Some signal processes need a different sample rate from the rest of an
instrument. Csound 7 lets a user-defined opcode (UDO) change its local rate
with `oversample` or `undersample`. The engine converts signals at the UDO
inputs and outputs.

This gives a filter or oscillator a local rate without a change to the
whole orchestra. The converter choice affects both the signal and the
processing cost.

## Increase the rate inside a UDO

This orchestra runs an oscillator at twice the parent sample rate:

```csound
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode LocalOscillator(frequency:i):a
  oversample 2
  signal:a = vco2(0.1, frequency)
  xout signal
endop

instr Tone
  signal:a = LocalOscillator(440)
  out signal
endin

schedule(Tone, 0, 0.25)
```

Inside the UDO, `sr` is `96000` and `kr` is `3000`.
The local `ksmps` remains `32`. The engine performs two local control cycles
for each parent cycle, then converts the output to the parent rate.

Oversampling can help processes that produce frequencies above the parent
Nyquist frequency. It also increases computation. Compare the sound and
processing cost at the rates that your application needs.

## Reduce the rate inside a UDO

`undersample` reduces both the local sample rate and the local block size.
For a parent rate of `48000` and `ksmps = 32`, a factor of `2` gives:

| Variable | Parent | Inside the UDO |
| --- | --- | --- |
| `sr` | `48000` | `24000` |
| `ksmps` | `32` | `16` |
| `kr` | `1500` | `1500` |

Choose a factor that divides the parent block size for an exact ratio.
Otherwise, Csound rounds the local block size and adjusts the rate to match.
The local block must contain at least one sample.

## Choose converters and keep rates separate

Both opcodes accept optional input and output converter settings.
With libsamplerate, settings `0`, `1`, and `2` select sinc converters with
different quality and cost. These converters add latency.
Settings `3` and `4` select zero-order hold and linear conversion.
The output converter defaults to the input converter.
Builds without libsamplerate use linear conversion for all converter settings.

Use positive integer factors. A factor of `1` leaves the rate unchanged.
Place rate setup before signal processing in the UDO.
Do not combine it with a different local `ksmps` from `setksmps`.

Pass signals through the UDO arguments. Global audio variables belong to
the parent rate and prevent a local sample-rate change. Audio and control
array arguments are supported.

Both classic and new UDO declarations support local rates. Rate changes
require argument copies and conversion, including with the new declaration
form. See the [resampling examples](../../tests/commandline/test_oversample.csd)
and [rate checks](../../tests/commandline/test_udo_resampling_arguments.csd)
for the supported behavior.
