<CsTest>
description = "Random audio generators follow current amplitude and frequency samples, including reused inputs"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 1
gkCompleted init 0

opcode Reference, aaa, aai
  setksmps 1
  aAmplitude, aFrequency, iSize xin
  kAmplitude downsamp aAmplitude
  kFrequency downsamp aFrequency
  kHold randh kAmplitude, kFrequency, .5, iSize
  kLinear randi kAmplitude, kFrequency, .5, iSize
  kCubic randc kAmplitude, kFrequency, .5, iSize
  aHold = kHold
  aLinear = kLinear
  aCubic = kCubic
  xout aHold, aLinear, aCubic
endop

instr CheckControls
  aActive = 1
  aAmplitude init 0
  aFrequency init 0
  kElapsed init 0
  kSample = 0
  kNext = kElapsed
  while kSample < ksmps do
    kActive vaget kSample, aActive
    kAmplitude = 0
    kFrequency = 0
    if kActive != 0 then
      kAmplitude = .25 + (kNext % 5)/8
      kFrequency = 2 + 2*(kNext % 3)
      kNext += 1
    endif
    vaset kAmplitude, kSample, aAmplitude
    vaset kFrequency, kSample, aFrequency
    kSample += 1
  od
  aHoldReference, aLinearReference, aCubicReference Reference aAmplitude, aFrequency, p4
  if p5 == 0 then
    aHold randh aAmplitude, aFrequency, .5, p4
    aLinear randi aAmplitude, aFrequency, .5, p4
    aCubic randc aAmplitude, aFrequency, .5, p4
  else
    ; Overwriting the frequency must not change the phase increment.
    aHold = aFrequency
    aLinear = aFrequency
    aCubic = aFrequency
    aHold randh aAmplitude, aHold, .5, p4
    aLinear randi aAmplitude, aLinear, .5, p4
    aCubic randc aAmplitude, aCubic, .5, p4
  endif
  kSample = 0
  while kSample < ksmps do
    kHold vaget kSample, aHold
    kLinear vaget kSample, aLinear
    kCubic vaget kSample, aCubic
    kHoldReference vaget kSample, aHoldReference
    kLinearReference vaget kSample, aLinearReference
    kCubicReference vaget kSample, aCubicReference
    if !(abs(kHold-kHoldReference) < .00001 && \
         abs(kLinear-kLinearReference) < .00001 && \
         abs(kCubic-kCubicReference) < .00001) then
      printks "size=%g reused=%g sample=%g: expected %g/%g/%g, got %g/%g/%g\n", \
        0, p4, p5, kSample, kHoldReference, kLinearReference, kCubicReference, kHold, kLinear, kCubic
      exitnowk -1
    endif
    kSample += 1
  od
  kElapsed = kNext
  if kElapsed == 24 then
    gkCompleted += 1
  endif
endin

instr CheckResults
  if i(gkCompleted) != 4 then
    prints "Audio control checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckControls" 0 .75 0 0
i "CheckControls" 0 .75 1 0
i "CheckControls" [1+3/32] .75 0 1
i "CheckControls" [1+3/32] .75 1 1
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
