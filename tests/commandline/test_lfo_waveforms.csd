<CsTest>
description = "lfo preserves waveform shape while stopping, reversing, and wrapping"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckWaveform
  iWaveform = p4
  iFrequency = p5
  iStep = p6
  iTwoPi = 8*taninv(1)
  kSample init 0

  ; Move forward for 16 samples, hold for 16, reverse for 16, then advance.
  ; Count the net forward steps without using the oscillator's phase state.
  if kSample < 16 then
    kFrequency = iFrequency
    kSteps = kSample
  elseif kSample < 32 then
    kFrequency = 0
    kSteps = 16
  elseif kSample < 48 then
    kFrequency = -iFrequency
    kSteps = 48-kSample
  else
    kFrequency = iFrequency
    kSteps = kSample-48
  endif
  kPhase = kSteps*iStep - floor(kSteps*iStep)
  kAmplitude = (kSample < 32 ? .75 : .5)
  aSignal lfo kAmplitude, kFrequency, iWaveform
  kSignal lfo kAmplitude, kFrequency, iWaveform
  kAudio downsamp aSignal

  if iWaveform == 0 then
    kExpected = sin(iTwoPi*kPhase)
  elseif iWaveform == 1 then
    kExpected = (kPhase < .25 ? 4*kPhase : (kPhase < .75 ? 2-4*kPhase : 4*kPhase-4))
  elseif iWaveform == 2 then
    kExpected = (kPhase < .5 ? 1 : -1)
  elseif iWaveform == 3 then
    kExpected = (kPhase < .5 ? 1 : 0)
  elseif iWaveform == 4 then
    kExpected = kPhase
  else
    kExpected = 1-kPhase
  endif
  kExpected *= kAmplitude
  ; Allow for the sine table's linear interpolation and single precision.
  if !(abs(kAudio-kExpected) < .000001 && abs(kSignal-kExpected) < .000001) then
    printks "lfo type=%g frequency=%g sample=%g expected=%g audio=%g control=%g\n", \
      0, iWaveform, iFrequency, kSample, kExpected, kAudio, kSignal
    exitnowk(-1)
  endif
  kSample += 1
  if kSample == 64 then
    gkChecks += 1
  endif
endin

instr CheckResults
  if i(gkChecks) != 18 then
    prints "lfo completed %g waveform checks; expected 18\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; At three sixteenths of a cycle per sample, visit all waveform corners.
;                                  type frequency     phase step
i "CheckWaveform" 0 [64/32768]       0    [32768*3/16]  [3/16]
i "CheckWaveform" 0 [64/32768]       1    [32768*3/16]  [3/16]
i "CheckWaveform" 0 [64/32768]       2    [32768*3/16]  [3/16]
i "CheckWaveform" 0 [64/32768]       3    [32768*3/16]  [3/16]
i "CheckWaveform" 0 [64/32768]       4    [32768*3/16]  [3/16]
i "CheckWaveform" 0 [64/32768]       5    [32768*3/16]  [3/16]
; Whole cycles added to the increment must not change the output.
i "CheckWaveform" 0 [64/32768]       0    [32768*(1024+3/16)] [3/16]
i "CheckWaveform" 0 [64/32768]       1    [32768*(1024+3/16)] [3/16]
i "CheckWaveform" 0 [64/32768]       2    [32768*(1024+3/16)] [3/16]
i "CheckWaveform" 0 [64/32768]       3    [32768*(1024+3/16)] [3/16]
i "CheckWaveform" 0 [64/32768]       4    [32768*(1024+3/16)] [3/16]
i "CheckWaveform" 0 [64/32768]       5    [32768*(1024+3/16)] [3/16]
; Reverse from phase zero, crossing the sine table's interpolated last cell.
i "CheckWaveform" 0 [64/32768]       0    -4            [-1/8192]
i "CheckWaveform" 0 [64/32768]       1    -4            [-1/8192]
i "CheckWaveform" 0 [64/32768]       2    -4            [-1/8192]
i "CheckWaveform" 0 [64/32768]       3    -4            [-1/8192]
i "CheckWaveform" 0 [64/32768]       4    -4            [-1/8192]
i "CheckWaveform" 0 [64/32768]       5    -4            [-1/8192]
i "CheckResults" [80/32768] [1/32768]
e
</CsScore>
</CsoundSynthesizer>
