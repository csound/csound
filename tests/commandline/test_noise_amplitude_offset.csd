<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giWhite ftgen 0, 0, 64, -2, 0
gkChecks init 0

instr 1
  ; Record one seeded white sequence for the filter reference below.
  seed 1234
  aWhite noise 1, 0
  kCount init 0
  kN = 0
  while kN < ksmps do
    kWhite vaget kN, aWhite
    tablew kWhite, kCount + kN, giWhite
    kN += 1
  od
  kCount += ksmps
endin

instr 2
  seed 1234
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kState init 0
  kBlock init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  kBeta = p5
  if p6 != 0 && kBlock >= 2 then
    kBeta = -.25
  endif
  kAmplitude = .25 + kBlock/8
  aAmplitude init 0
  kN = 0
  while kN < ksmps do
    kValue = 0
    if kN >= kStart && kN < kEnd then
      kValue = .25 + (kCount + kN - kStart)/64
    endif
    vaset kValue, kN, aAmplitude
    kN += 1
  od
  if p4 == 0 then
    aActual noise kAmplitude, kBeta
  elseif p4 == 1 then
    aActual noise aAmplitude, kBeta
  else
    aAmplitude noise aAmplitude, kBeta
    aActual = aAmplitude
  endif
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kWhite table kSample, giWhite
      kState = kBeta*kState + sqrt(1 - kBeta*kBeta)*kWhite
      kAmp = (p4 == 0 ? kAmplitude : .25 + kSample/64)
      kExpected = kState*kAmp/(1 + kBeta)
    endif
    kActual vaget kN, aActual
    if !(abs(kActual - kExpected) < .00001) then
      printks "FAIL noise mode=%g offset=%g beta=%g sample=%g: %g expected %g\n", \
          0, p4, iOffset, kBeta, kCount + kN - kStart, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  kBlock += 1
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 12 then
    prints "FAIL noise checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
; Aligned and partial blocks, all amplitude paths, and both filter signs.
i 2 .125 .0625 0 0 0
i 2 .2529296875 .0625 0 .5 0
i 2 .3779296875 .0625 0 -.5 0
i 2 .5 .0625 1 0 0
i 2 .6279296875 .0625 1 .5 0
i 2 .7529296875 .0625 1 -.5 0
i 2 .875 .0625 2 0 0
i 2 1.0029296875 .0625 2 .5 0
i 2 1.1279296875 .0625 2 -.5 0
; A coefficient change must keep the filter history.
i 2 1.2529296875 .0625 0 .5 1
i 2 1.3779296875 .0625 1 .5 1
i 2 1.5029296875 .0625 2 .5 1
i 99 1.625 .015625
</CsScore>
</CsoundSynthesizer>
