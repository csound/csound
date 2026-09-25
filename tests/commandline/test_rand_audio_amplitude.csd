<CsTest>
description = "randh, randi, and randc use the active audio amplitude samples"
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

instr CheckAmplitude
  setksmps p5
  aActive = 1
  aAmplitude = 0.5
  aHold randh aAmplitude, 4, .5, p4
  aLinear randi aAmplitude, 4, .5, p4
  aCubic randc aAmplitude, 4, .5, p4
  aHoldReference randh .5, 4, .5, p4
  aLinearReference randi .5, 4, .5, p4
  aCubicReference randc .5, 4, .5, p4
  kElapsed init 0
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    kHold vaget kSample, aHold
    kLinear vaget kSample, aLinear
    kCubic vaget kSample, aCubic
    kHoldReference vaget kSample, aHoldReference
    kLinearReference vaget kSample, aLinearReference
    kCubicReference vaget kSample, aCubicReference
    if !(abs(kHold-kHoldReference) < .00001 && \
         abs(kLinear-kLinearReference) < .00001 && \
         abs(kCubic-kCubicReference) < .00001) then
      printks "size=%g local ksmps=%g sample=%g: audio amplitudes %g/%g/%g; scalar amplitudes %g/%g/%g\n", \
        0, p4, p5, kSample, kHold, kLinear, kCubic, kHoldReference, kLinearReference, kCubicReference
      exitnowk -1
    endif
    if kActive != 0 then
      kElapsed += 1
    endif
    kSample += 1
  od
  if kElapsed == 16 then
    gkCompleted += 1
  endif
endin

instr CheckResults
  if i(gkCompleted) != 6 then
    prints "Audio amplitude checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Both generators, aligned blocks, partial blocks, and local blocks.
i "CheckAmplitude" 0 .5 0 8
i "CheckAmplitude" 0 .5 1 8
i "CheckAmplitude" [.5+3/32] .5 0 8
i "CheckAmplitude" [.5+3/32] .5 1 8
i "CheckAmplitude" [1.25+1/32] .5 0 2
i "CheckAmplitude" [1.25+1/32] .5 1 2
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
