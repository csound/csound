<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giFour ftgen 1, 0, 4, -2, .125, .25, .375, .5
giThree ftgen 2, 0, -3, -2, .25, .5, .75
giOne ftgen 3, 0, -1, -2, .75
gkChecks init 0

instr 1
  iLength = ftlen(p4)
  iRepeats = int(p6)
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  kAmp = .5 + kCount/128
  aOut osciln kAmp, p5*sr, p4, p6
  aAlias oscilx kAmp, p5*sr, p4, p6
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kAlias vaget kN, aAlias
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kCycles = kSample*p5
      if kCycles < iRepeats then
        kPhase = kCycles - floor(kCycles)
        kIndex = int(kPhase*iLength)
        kWave table kIndex, p4
        kExpected = kAmp*kWave
      endif
    endif
    if !(abs(kActual - kExpected) < .000001) || \
       !(abs(kAlias - kExpected) < .000001) then
      printks "FAIL osciln table=%g step=%g repeats=%g offset=%g sample=%g: %g/%g expected %g\n", \
          0, p4, p5, p6, iOffset, kCount + kN - kStart, kActual, kAlias, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 2
  kBlock init 0
  kAge init 0
  if kBlock == 2 then
    kAge = 0
    reinit OSC
  endif
OSC:
  aOut osciln 1, sr/4, giFour, 2
  rireturn
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kSample = kAge + kN
    kExpected = 0
    if kSample < 8 then
      kExpected = .125*(1 + kSample % 4)
    endif
    if kActual != kExpected then
      printks "FAIL osciln reinit sample=%g: %g expected %g\n", 0, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBlock += 1
  kAge += ksmps
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 17 then
    prints "FAIL osciln checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; One/two repeats, last samples at and inside block boundaries.
i 1 0 .0625 1 .25 1
i 1 .125 .0625 1 .25 2
i 1 .25 .0625 1 .25 4
i 1 .3779296875 .0625 1 .25 4
; Fractional table steps and non-power-of-two/one-entry tables.
i 1 .5 .0625 1 .125 2
i 1 .6279296875 .0625 1 .1875 3
i 1 .75 .0625 2 .25 2
i 1 .8779296875 .0625 3 .25 2
; More than one cycle per sample, including an advance past all repeats.
i 1 1 .0625 1 1.5 5
i 1 1.1279296875 .0625 2 2.5 5
i 1 1.25 .0625 1 8 2
i 1 1.375 .0625 1 1099511627776 2
; Zero repeats are silent; zero frequency holds the first table entry.
i 1 1.5029296875 .0625 1 .25 0
i 1 1.625 .0625 1 0 2
i 1 1.7529296875 .0625 2 .25 2.75
; Continue phase and remaining repeats across several blocks.
i 1 1.8779296875 .0625 2 .125 8
i 2 2 .0625
i 99 2.125 .015625
</CsScore>
</CsoundSynthesizer>
