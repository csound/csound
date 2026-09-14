<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giPower ftgen 1, 0, 128, 10, 1
giOther ftgen 2, 0, -96, 10, 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  iHarmonics = max(1, int(abs(p6)))
  kPhase init p5 - floor(p5)
  kCount init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  aAmplitude init 0
  aFrequency init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN - kStart
    vaset .25 + kSample/128, kN, aAmplitude
    vaset p8*sr*(1 + kSample % 2), kN, aFrequency
    kN += 1
  od
  if p7 == 0 then
    aOut buzz .5, p8*sr, p6, p4, p5
  elseif p7 == 1 then
    aOut buzz aAmplitude, aFrequency, p6, p4, p5
  else
    aAmplitude buzz aAmplitude, aFrequency, p6, p4, p5
    aOut = aAmplitude
  endif
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kAmp = (p7 == 0 ? .5 : .25 + kSample/128)
      kStep = (p7 == 0 ? p8 : p8*(1 + kSample % 2))
      ; buzz is the equal-amplitude sum of cosine harmonics.
      ; These phases lie on both sine tables' sample grids.
      kH = 1
      while kH <= iHarmonics do
        kExpected += cos(2*$M_PI*kH*kPhase)*kAmp/iHarmonics
        kH += 1
      od
      kPhase += kStep
      kPhase -= floor(kPhase)
    endif
    if !(abs(kActual - kExpected) < .00001) then
      printks "FAIL buzz table=%g phase=%g harmonics=%g mode=%g sample=%g: %g expected %g\n", \
          0, p4, p5, p6, p7, kCount + kN - kStart, kActual, kExpected
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
  aReference buzz .5, 128, 3, p4, .25
  kBlock init 0
  kPhase init .25
  if kBlock == 2 then
    kPhase = -1
    reinit OSC
  endif
OSC:
  aActual buzz .5, 128, 3, p4, i(kPhase)
  rireturn
  kError max_k abs(aActual - aReference), 1, 1
  if !(kError < .000001) then
    printks "FAIL buzz negative initial phase reset the oscillator\n", 0
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 3
  ; A reverse step smaller than float spacing must still move off the
  ; exact table grid point. The next 63 samples stay in the same cells.
  iNumerator table 83, giOther
  iDenominator table 11, giOther
  iExpected = (iNumerator/iDenominator - 1)/6
  aOut buzz 1, -sr/268435456, 3, giOther, .25
  kCount init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = (kCount + kN == 0 ? -1/3 : iExpected)
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL buzz lost a small phase increment: %g expected %g\n", 0, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 21 then
    prints "FAIL buzz checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Control inputs, audio inputs, and input/output reuse.
i 1 0 .0625 1 0 3 0 .125
i 1 .1279296875 .0625 2 0 3 0 .125
i 1 .2529296875 .0625 1 0 3 1 .125
i 1 .3779296875 .0625 2 0 3 1 .125
i 1 .5029296875 .0625 1 0 3 2 .125
i 1 .6279296875 .0625 2 0 3 2 .125
; Initial phase must agree across table types, including whole cycles.
i 1 .75 .0625 1 .25 3 0 .125
i 1 .8779296875 .0625 2 .25 3 0 .125
i 1 1 .0625 1 1.25 3 1 .125
i 1 1.1279296875 .0625 2 1.25 3 1 .125
; Negative counts use their absolute value; zero means one harmonic.
i 1 1.25 .0625 1 0 -3 1 .125
i 1 1.3779296875 .0625 2 0 -3 1 .125
i 1 1.5 .0625 1 .25 0 0 .125
i 1 1.6279296875 .0625 2 .25 0 0 .125
; Reverse playback, whole cycles, and stationary phase at the peak.
i 1 1.75 .0625 2 .25 3 1 -.125
i 1 1.8779296875 .0625 2 .25 3 1 -2
i 1 2 .0625 1 0 3 1 0
i 1 2.1279296875 .0625 2 0 3 2 0
i 2 2.25 .0625 1
i 2 2.375 .0625 2
i 3 2.5 .0625
i 99 2.625 .015625
</CsScore>
</CsoundSynthesizer>
