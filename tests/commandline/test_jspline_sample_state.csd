<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1

; Knots from seed 1234, including the lookahead used for the last slope.
giKnots[] fillarray -.1217988464617165, .5376942672476611, \
    .5875398137548659, .762037805636431, -.04141531234673006, \
    .9114034692344272, -.7846320656987987, -.8093137232630112, \
    -.6111656216025192, .4463828124321917
gkChecked init 0

opcode Reference, k, k
  kSample xin
  kSegment = int(kSample / 4)
  kX = (kSample % 4) / 4
  kStart = giKnots[kSegment]
  kEnd = giKnots[kSegment + 1]
  kSlope0 = 0
  if kSegment > 0 then
    kSlope0 = (kEnd - giKnots[kSegment - 1]) / 2
  endif
  kSlope1 = (giKnots[kSegment + 2] - kStart) / 2
  ; Cubic Hermite interpolation of the seeded knots and slopes.
  kValue = (2*kX^3 - 3*kX^2 + 1)*kStart + \
           (kX^3 - 2*kX^2 + kX)*kSlope0 + \
           (-2*kX^3 + 3*kX^2)*kEnd + (kX^3 - kX^2)*kSlope1
  xout kValue
endop

instr 1
  seed 1234
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 32 - kCount)
  aAmplitude init 0
  kN = 0
  while kN < ksmps do
    kAmplitude = 0
    if kN >= kStart && kN < kEnd then
      kAmplitude = .5 + (kCount + kN - kStart)/64
    endif
    vaset kAmplitude, kN, aAmplitude
    kN += 1
  od
  if p4 == 1 then
    aOutput jspline aAmplitude, sr/4, sr/4
  elseif p4 == 2 then
    aAmplitude jspline aAmplitude, sr/4, sr/4
    aOutput = aAmplitude
  else
    aOutput jspline .75, sr/4, sr/4
  endif
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOutput
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kSpline Reference kSample
      kAmplitude = (p4 == 3 ? .75 : .5 + kSample/64)
      kExpected = kSpline*kAmplitude
      gkChecked += 1
    endif
    if abs(kActual - kExpected) > .00001 then
      printks "FAIL jspline mode=%g offset=%g sample=%g: %g expected %g\n", \
          0, p4, iOffset, kCount + kN - kStart, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
endin

instr 2
  seed 1234
  kCount init 0
  kActual jspline 1, kr/4, kr/4
  kExpected Reference kCount
  if abs(kActual - kExpected) > .00001 then
    printks "FAIL control jspline cycle=%g: %g expected %g\n", \
        0, kCount, kActual, kExpected
    exitnowk -1
  endif
  kCount += 1
  gkChecked += 1
endin

instr 99
  if gkChecked != 160 then
    printks "FAIL checked %g samples, expected 160\n", 0, gkChecked
    exitnowk -1
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 1
i 1 .0654296875 .03125 1
i 1 .1298828125 .03125 2
i 1 .1943359375 .03125 3
i 2 .25 .5
i 99 .8 .015625
</CsScore>
</CsoundSynthesizer>
