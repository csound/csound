<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

; Constant tables make grain duration and amplitude directly observable.
giPowerOfTwo ftgen 1, 0, 128, 7, 1, 128, 1
giNonPowerOfTwo ftgen 2, 0, 100, 7, 1, 100, 1

opcode assert_sample, 0, kk
  kActual, kExpected xin
  ; This comparison also rejects NaN and infinity.
  if !(abs(kActual - kExpected) <= 0.00001) then
    printks "fof/fof2 sample mismatch: actual=%.9f expected=%.9f\n", 0, kActual, kExpected
    exitnowk(-1)
  endif
endop

instr 1
  ; p4: duration, p5: table, p6: expected number of grain samples.
  aFof fof 1, 0, 0, 0, 0, 0, p4, 0, 1, p5, p5, p3
  aFof2 fof2 1, 0, 0, 0, 0, 0, p4, 0, 1, p5, p5, p3, 0, 0
  kCount init 0
  kExpected = (kCount < p6 ? 1 : 0)
  assert_sample(downsamp(aFof), kExpected)
  assert_sample(downsamp(aFof2), kExpected)
  kCount += 1
endin

instr 2
  ; A one-sample grain every four samples exposes the octave mask.
  ; Fractional octaves attenuate every other surviving grain.
  aFof fof 1, sr/4, 0, p4, 0, 0, 1/sr, 0, 1, p5, p5, p3
  aFof2 fof2 1, sr/4, 0, p4, 0, 0, 1/sr, 0, 1, p5, p5, p3, 0, 0
  iStride = 2 ^ int(p4)
  kCount init 0
  kPulse = int(kCount / 4)
  kExpected = 0
  if kCount % 4 == 0 && kPulse % iStride == 0 then
    kExpected = 1
    if int(kPulse / iStride) % 2 == 1 then
      kExpected = 1 - frac(p4)
    endif
  endif
  assert_sample(downsamp(aFof), kExpected)
  assert_sample(downsamp(aFof2), kExpected)
  kCount += 1
endin

instr 3
  ; Negative phase on a fresh instance must still allocate overlaps.
  aFof fof 1, sr/4, 0, 0, 0, 0, 1/sr, 0, p5, p4, p4, p3, -0.25
  aFof2 fof2 1, sr/4, 0, 0, 0, 0, 1/sr, 0, p5, p4, p4, p3, -0.25, 0
  kCount init 0
  kExpected = (kCount % 4 == 1 ? 1 : 0)
  assert_sample(downsamp(aFof), kExpected)
  assert_sample(downsamp(aFof2), kExpected)
  kCount += 1
endin

instr 4
  ; Exercise zero fixed-point rise increments and float rise counts
  ; beyond INT32_MAX. With zero bandwidth the constant grain stays at 1.
  aFof fof 1, 0, 100, 0, 0, p4, 0.00025, 0, 1, p5, p5, p3
  aFof2 fof2 1, 0, 100, 0, 0, p4, 0.00025, 0, 1, p5, p5, p3, 0, 0
  kCount init 0
  kExpected = (kCount < 12 ? 1 : 0)
  assert_sample(downsamp(aFof), kExpected)
  assert_sample(downsamp(aFof2), kExpected)
  kCount += 1
endin
</CsInstruments>
<CsScore>
; Ordinary durations must keep their sample counts in float builds too.
i1 0 0.022 0.01 1 480
i1 0 0.022 0.02 1 960
i1 0 0.022 0.01 2 480
i1 0 0.022 0.02 2 960
; Sub-sample and zero durations must retire after one sample.
i1 0 0.001 0.00001 1 1
i1 0 0.001 0.00001 2 1
i1 0 0.001 0 1 1
i1 0 0.001 0 2 1
; Integer and fractional octaves on both table paths.
i2 0 0.01 0 1
i2 0 0.01 0.5 1
i2 0 0.01 1 1
i2 0 0.01 1.5 1
i2 0 0.01 2 1
i2 0 0.01 2.5 1
i2 0 0.01 63 1
i2 0 0.01 64 1
i2 0 0.01 0 2
i2 0 0.01 0.5 2
i2 0 0.01 1 2
i2 0 0.01 1.5 2
i2 0 0.01 2 2
i2 0 0.01 2.5 2
i2 0 0.01 63 2
i2 0 0.01 64 2
i3 0 0.001 1 1
i3 0 0.001 2 1
; Reused negative-phase instances may need larger overlap buffers.
i3 0.002 0.001 1 8
i3 0.002 0.001 2 8
i4 0 0.001 0.001 1
i4 0 0.001 22370 1
i4 0 0.001 0.001 2
i4 0 0.001 50000 2
e
</CsScore>
</CsoundSynthesizer>
