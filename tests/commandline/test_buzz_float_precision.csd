<CsTest>
description = "buzz retains phase detail and integer harmonics in float builds"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giSine ftgen 1, 0, -96, 10, 1
giOdd ftgen 2, 0, -97, 10, 1
gkChecks init 0

instr 1
  ; Move off a table boundary in reverse, or cross it after 16 tiny steps.
  ; Derive the reference cells from the sample count, not a float phase sum.
  iPhase = (p4 < 0 ? .25 : .25 - 1/16777216)
  iFrequency = p4*sr/268435456
  aAmplitude = 1
  aFrequency = iFrequency
  if p5 == 0 then
    aOut buzz 1, iFrequency, 3, giSine, iPhase
  elseif p5 == 1 then
    aOut buzz aAmplitude, aFrequency, 3, giSine, iPhase
  else
    aAmplitude buzz aAmplitude, aFrequency, 3, giSine, iPhase
    aOut = aAmplitude
  endif
  kCount init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    kBelow = (p4 < 0 ? kSample > 0 : kSample < 16)
    kDenominator table 12 - kBelow, giSine
    kNumerator table 84 - kBelow, giSine
    kExpected = (kNumerator/kDenominator - 1)/6
    kActual vaget kN, aOut
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL buzz small step direction=%g mode=%g sample=%g: %g expected %g\n", \
          0, p4, p5, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 2
  ; Each exact integer 2*n+1 is 1 modulo 8, even when float rounds it.
  ; At phase 1/8 the two sine lookups must therefore use the same cell.
  aOut buzz 1, 0, p4, giSine, .25
  kCount init 0
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    ; Undo amplitude scaling so an incorrect numerator cannot hide near zero.
    if !(abs(kActual*(2*p4)) < .000002) then
      printks "FAIL buzz rounded harmonic multiplier for n=%g: scaled output %g\n", \
          0, p4, kActual*(2*p4)
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 3
  ; (2*16777216+1)*2^-29 = 1/16 + 2^-29. Keep the small part
  ; when it selects the cell just below a reverse harmonic boundary.
  aFrequency = p4*sr/268435456
  aOut buzz 1, aFrequency, 16777216, giSine, .25
  kCount init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    kBelow = (p4 < 0 && kSample > 0 ? 1 : 0)
    kIndex = (12 + p4*6*kSample - kBelow) % 96
    kIndex = (kIndex + 96) % 96
    kNumerator table kIndex, giSine
    kDenominator table 12 - kBelow, giSine
    kExpected = kNumerator/kDenominator - 1
    kActual vaget kN, aOut
    kActual *= 33554432
    if !(abs(kActual - kExpected) < .000002) then
      printks "FAIL buzz large harmonic direction=%g sample=%g: %g expected %g\n", \
          0, p4, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 4
  ; A large increment must retain the earlier tiny reverse step.
  ; Reuse the audio frequency buffer as the output as well.
  kCount init 0
  aFrequency init 0
  kN = 0
  while kN < ksmps do
    vaset (kCount + kN == 0 ? -sr/268435456 : sr*.5), kN, aFrequency
    kN += 1
  od
  aFrequency buzz 1, aFrequency, 3, giSine, .25
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    if kSample == 0 then
      kDenIndex = 12
      kNumIndex = 84
    else
      kDenIndex = (12 + 24*(kSample - 1) - 1) % 96
      kNumIndex = (84 + 168*(kSample - 1) - 1) % 96
    endif
    kDenominator table kDenIndex, giSine
    kNumerator table kNumIndex, giSine
    kExpected = (kNumerator/kDenominator - 1)/6
    kActual vaget kN, aFrequency
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL buzz changing frequency sample=%g: %g expected %g\n", \
          0, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 5
  ; Reinit must keep the saved remainder for negative phase, and clear it
  ; for a fresh start. This step leaves a nonzero remainder after one block.
  kCount init 0
  kPhase init .25
  if kCount == 16 then
    kPhase = (p4 == 0 ? -1 : .25)
    reinit OSC
  endif
OSC:
  aOut buzz 1, -sr/4294967296, 3, giSine, i(kPhase)
  rireturn
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    kBelow = (kSample > 0 && !(p4 == 1 && kSample == 16) ? 1 : 0)
    kDenominator table 12 - kBelow, giSine
    kNumerator table 84 - kBelow, giSine
    kExpected = (kNumerator/kDenominator - 1)/6
    kActual vaget kN, aOut
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL buzz reinit reset=%g sample=%g: %g expected %g\n", \
          0, p4, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 6
  ; This step leaves both products below a cell boundary on the odd table.
  ; On the other table, check small steps that survive larger cancelling ones.
  iTable = (p4 == 0 ? giOdd : giSine)
  iPhase = (p4 == 0 ? 11069503/268435456 : .25)
  kCount init 0
  aFrequency init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    kStep = 0
    if p4 == 0 && kSample == 0 then
      kStep = 43/68719476736
    elseif p4 == 1 then
      kStep = (kSample == 0 ? -2^(-53) : (kSample == 1 ? 2^(-27) : (kSample == 2 ? -2^(-27) : 0)))
    elseif p4 == 2 then
      if kSample == 0 then
        kStep = 2^(-100)
      elseif kSample == 1 then
        kStep = 2^(-53)
      elseif kSample == 2 then
        kStep = 2^(-27)
      elseif kSample == 3 then
        kStep = -2^(-27)
      elseif kSample == 4 then
        kStep = -2^(-53)
      endif
    endif
    vaset kStep*2*sr, kN, aFrequency
    kN += 1
  od
  aOut buzz 1, aFrequency, 3, iTable, iPhase
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    kBelow = (p4 == 1 && kSample != 0 && kSample != 2 ? 1 : 0)
    kDenominator table (p4 == 0 ? 1 : 12 - kBelow), iTable
    kNumerator table (p4 == 0 ? 13 : 84 - kBelow), iTable
    kExpected = (kNumerator/kDenominator - 1)/6
    kActual vaget kN, aOut
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL buzz cell boundary case=%g sample=%g: %g expected %g\n", \
          0, p4, kSample, kActual, kExpected
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
  if i(gkChecks) != 17 then
    prints "FAIL buzz float checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625 -1 0
i 1 .125 .0625 -1 1
i 1 .25 .0625 -1 2
i 1 .375 .0625 1 0
i 1 .5 .0625 1 1
i 1 .625 .0625 1 2
i 2 .75 .0625 8388608
i 2 .875 .0625 16777216
i 2 1 .0625 2147483520
i 3 1.125 .0625 -1
i 3 1.25 .0625 1
i 4 1.375 .0625
i 5 1.5 .0625 0
i 5 1.625 .0625 1
i 6 1.75 .0625 0
i 6 1.875 .0625 1
i 6 2 .0625 2
i 99 2.125 .015625
</CsScore>
</CsoundSynthesizer>
