<CsTest>
description = "Sliding pvsanal removes old samples without accumulating rotation error"
[expect]
exit = 0
</CsTest>
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

instr 1
  iSize = p4
  kCount init 0
  aInput init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount+kN
    ; A long varying input exercises entering and leaving nonzero samples.
    kValue = (kSample < 4096 ? (kSample%53-26)/32+sin((kSample%127)*.1) : 0)
    vaset kValue, kN, aInput
    kN += 1
  od
  fAnalysis pvsanal aInput, iSize, 1, iSize, p5
  if kCount >= 4096+iSize then
    kBin = 0
    while kBin <= iSize/2 do
      aMagnitude, aFrequency pvsbin fAnalysis, kBin
      kN = 0
      while kN < ksmps do
        kMagnitude vaget kN, aMagnitude
        if !(abs(kMagnitude) < .0001) then
          printks "FAIL pvsanal size=%g window=%g bin=%g left magnitude %.12g after silence\n", 0, iSize, p5, kBin, kMagnitude
          exitnowk -1
        endif
        kN += 1
      od
      kBin += 1
    od
  endif
  kCount += ksmps
  if kCount == 4096+2*iSize+32 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 9 then
    prints "FAIL pvsanal history checks did not complete: %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 64 9
i 1 0 1 64 1
i 1 0 1 64 4
i 1 0 1 96 9
i 1 0 1 96 1
i 1 0 1 96 4
i 1 0 1 1024 9
i 1 0 1 1024 1
i 1 0 1 1024 4
i 99 1.01 .001
e
</CsScore>
</CsoundSynthesizer>
