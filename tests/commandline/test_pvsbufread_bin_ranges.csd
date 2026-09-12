<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; Compare frequency ranges at a fixed frame, independent of circular wrapping.
instr 1
  kSource[] init 66
  kFirst[] init 66
  kRead[] init 66
  kCycle init 0
  aRamp phasor 137
  aTone oscili .2, 3072
  aInput = aTone+.01*(kCycle+1)*aRamp
  fInput pvsanal aInput, 64, 16, 64, 1
  iBuffer, kTime pvsbuffer fInput, 3/512
  kFrame pvs2tab kSource, fInput
  kIndex = 0
  while kIndex < 66 do
    if kTime == 0 then
      kFirst[kIndex] = kSource[kIndex]
    endif
    kIndex += 1
  od
  fRead pvsbufread 0, iBuffer, p4*128, p5*128, 1
  kFrame pvs2tab kRead, fRead
  kIndex = 0
  while kIndex < 66 do
    kBin = int(kIndex/2)
    kExpected = 0
    if kBin >= p4 && (p5 <= p4 || kBin <= p5) then
      kExpected = kFirst[kIndex]
    endif
    if !(abs(kRead[kIndex]-kExpected) <= .001) then
      printks "pvsbufread range %g..%g cycle %g index %g: got %g, expected %g\n", 0, p4, p5, kCycle, kIndex, kRead[kIndex], kExpected
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
  if i(gkChecks) != 3 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full spectrum, an odd-numbered high bin range, and Nyquist alone.
i 1 0 .125 0 0
i 1 0 .125 23 25
i 1 0 .125 32 32
i 99 .25 .01
e
</CsScore>
</CsoundSynthesizer>
