<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kBlock init 0
  kY1 init 0
  kY2 init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  kCutoff = 64 + 16*(kBlock % 3)
  kQ = 1 + .5*(kBlock % 3)
  aInput init 0
  aCutoff init 0
  aQ init 0
  kN = 0
  while kN < ksmps do
    kInput = 0
    kF = 0
    kRes = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kInput = .25 + kSample/256
      kF = 64 + 16*(kSample % 5)
      kRes = 1 + .5*(kSample % 3)
    endif
    vaset kInput, kN, aInput
    vaset kF, kN, aCutoff
    vaset kRes, kN, aQ
    kN += 1
  od
  if p5 != -99 && kBlock == 2 then
    if p5 == 0 then
      kY1 = 0
      kY2 = 0
    endif
    reinit FILTER
  endif
FILTER:
  iSkip = (i(kBlock) == 0 ? 0 : p5)
  if p5 == -99 then
    if p4 == 0 then
      aOut lowpass2 aInput, kCutoff, kQ
    elseif p4 == 1 then
      aOut lowpass2 aInput, aCutoff, kQ
    elseif p4 == 2 then
      aOut lowpass2 aInput, kCutoff, aQ
    else
      aOut lowpass2 aInput, aCutoff, aQ
    endif
  else
    if p4 == 0 then
      aInput lowpass2 aInput, kCutoff, kQ, iSkip
    elseif p4 == 1 then
      aInput lowpass2 aInput, aCutoff, kQ, iSkip
    elseif p4 == 2 then
      aInput lowpass2 aInput, kCutoff, aQ, iSkip
    else
      aInput lowpass2 aInput, aCutoff, aQ, iSkip
    endif
    aOut = aInput
  endif
  rireturn
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kF = (p4 == 1 || p4 == 3 ? 64 + 16*(kSample % 5) : kCutoff)
      kRes = (p4 >= 2 ? 1 + .5*(kSample % 3) : kQ)
      kDecay = exp(-$M_PI*kF/(kRes*sr))
      kA = 2*cos(2*$M_PI*kF/sr)*kDecay
      kB = kDecay*kDecay
      kC = 1 - kA + kB
      kExpected = kA*kY1 - kB*kY2 + kC*(.25 + kSample/256)
      kY2 = kY1
      kY1 = kExpected
    endif
    kActual vaget kN, aOut
    if !(abs(kActual - kExpected) < .00002) then
      printks "FAIL lowpass2 rates=%g skip=%g sample=%g: %g expected %g\n", \
          0, p4, p5, kCount + kN - kStart, kActual, kExpected
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
  if i(gkChecks) != 13 then
    prints "FAIL lowpass2 checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; All rates with omitted iskip, then input reuse and reinit with reset/preserve.
i 1 0 .0625 0 -99
i 1 .1279296875 .0625 1 -99
i 1 .2529296875 .0625 2 -99
i 1 .3779296875 .0625 3 -99
i 1 .5029296875 .0625 0 0
i 1 .6279296875 .0625 1 0
i 1 .7529296875 .0625 2 0
i 1 .8779296875 .0625 3 0
i 1 1.0029296875 .0625 0 1
i 1 1.1279296875 .0625 1 1
i 1 1.2529296875 .0625 2 1
i 1 1.3779296875 .0625 3 1
i 1 1.5029296875 .0625 1 -1
i 99 1.625 .015625
</CsScore>
</CsoundSynthesizer>
