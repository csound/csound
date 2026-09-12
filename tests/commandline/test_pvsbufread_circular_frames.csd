<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
giZero ftgen 0, 0, -33, -2, 0
giQuarter ftgen 0, 0, -33, -7, 1/2048, 33, 1/2048
gkChecks init 0

; Interpolate between the last frame and the first, including negative times.
instr 2
  kSource[] init 66
  kFirst[] init 66
  kLast[] init 66
  kRead[] init 66
  kRead2[] init 66
  kCycle init 0
  aRamp phasor 137
  aInput = .01*(kCycle+1)*aRamp
  fInput pvsanal aInput, 64, 16, 64, 1
  iBuffer, kTime pvsbuffer fInput, 2/512
  kFrame pvs2tab kSource, fInput
  kIndex = 0
  while kIndex < 66 do
    if kTime == 0 then
      kFirst[kIndex] = kSource[kIndex]
    else
      kLast[kIndex] = kSource[kIndex]
    endif
    kIndex += 1
  od
  fRead pvsbufread p4/512, iBuffer
  fRead2 pvsbufread2 p4/512, iBuffer, giZero, giQuarter
  kFrame pvs2tab kRead, fRead
  kFrame pvs2tab kRead2, fRead2
  kIndex = 0
  while kIndex < 66 do
    kExpected = .75*kLast[kIndex]+.25*kFirst[kIndex]
    if kIndex % 2 == 0 then
      kExpected2 = kExpected
    else
      kExpected2 = kLast[kIndex]
    endif
    ; Check pvsbufread only below bin 16 so its separate range bug
    ; does not affect this frame-wrapping test. Check all pvsbufread2 bins.
    if !((kIndex >= 32 || abs(kRead[kIndex]-kExpected) <= .001) && abs(kRead2[kIndex]-kExpected2) <= .001) then
      printks "pvsbufread wrap cycle %g index %g: got %g / %g, expected %g / %g\n", 0, kCycle, kIndex, kRead[kIndex], kRead2[kIndex], kExpected, kExpected2
      exitnowk -1
    endif
    kIndex += 1
  od
  kCycle += 1
  if kCycle == 40 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 2 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 2 0 .125 1.25
i 2 0 .125 -.75
i 99 .25 .01
e
</CsScore>
</CsoundSynthesizer>
