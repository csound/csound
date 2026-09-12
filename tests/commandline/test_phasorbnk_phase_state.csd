<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kCycle init 0
  kExpected init (p5 == 1 || p5 < 0 ? 0 : p5)
  kIncrement = (kCycle < 4 ? p4 : -p4)
  kPhase phasorbnk kIncrement*kr, p7, p6, p5
  if !(abs(kPhase-kExpected) < .00001) then
    printks "phasorbnk control phase: got %g, expected %g\n", 0, kPhase, kExpected
    exitnowk -1
  endif
  kExpected += kIncrement-int(kIncrement)
  kExpected -= floor(kExpected)
  kCycle += 1
  if kCycle == 12 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  iOffset = int(p2*sr+.5) % ksmps
  iSamples = int(p3*sr+.5)
  kSamples init 0
  kExpectedK init (p5 == 1 ? 0 : p5)
  kExpectedA init (p5 == 1 ? 0 : p5)
  aRamp phasor sr/16
  aCps = (p4+aRamp)*sr
  aPhaseK phasorbnk p4*sr, 0, 2, p5
  aPhaseA phasorbnk aCps, 0, 2, p5
  aInPlace = aCps
  aInPlace phasorbnk aInPlace, 0, 2, p5
  kStart = (kSamples == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart+iSamples-kSamples)
  kIndex = 0
  while kIndex < ksmps do
    kPhaseK vaget kIndex, aPhaseK
    kPhaseA vaget kIndex, aPhaseA
    kInPlace vaget kIndex, aInPlace
    if kIndex >= kStart && kIndex < kEnd then
      if !(abs(kPhaseK-kExpectedK) < .00001 && abs(kPhaseA-kExpectedA) < .00001 && abs(kInPlace-kExpectedA) < .00001) then
        printks "phasorbnk audio phase at sample %g: got %g / %g / %g, expected %g / %g\n", 0, kSamples, kPhaseK, kPhaseA, kInPlace, kExpectedK, kExpectedA
        exitnowk -1
      endif
      kExpectedK += p4-int(p4)
      kExpectedK -= floor(kExpectedK)
      kIncrement = p4+(kSamples % 16)/16
      kExpectedA += kIncrement-int(kIncrement)
      kExpectedA -= floor(kExpectedA)
      kSamples += 1
    elseif kPhaseK != 0 || kPhaseA != 0 || kInPlace != 0 then
      printks "phasorbnk wrote outside the active samples\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  if kSamples == iSamples then
    gkChecks += 1
    turnoff
  endif
endin

; Keep old phases when growing, and start newly enabled banks at zero.
instr 3
  kCount init p4
  kInitial init .25
  kCycle init 0
  kExpected[] init 8
  iIndex = 0
  while iIndex < p4 do
    kExpected[iIndex] init .25
    iIndex += 1
  od
  if kCycle == 4 || kCycle == 8 then
    kNewCount = (kCycle == 4 ? p5 : p6)
    kIndex = kCount
    while kIndex < kNewCount do
      kExpected[kIndex] = 0
      kIndex += 1
    od
    kCount = kNewCount
    kInitial = -1
    reinit RESTART
  endif
  kIndex = kCycle % kCount
RESTART:
  kPhase phasorbnk kr/8, kIndex, i(kCount), i(kInitial)
  rireturn
  if !(abs(kPhase-kExpected[kIndex]) < .00001) then
    printks "phasorbnk reinit cycle %g bank %g: got %g, expected %g\n", 0, kCycle, kIndex, kPhase, kExpected[kIndex]
    exitnowk -1
  endif
  kExpected[kIndex] = frac(kExpected[kIndex]+.125)
  kCycle += 1
  if kCycle == 16 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 18 then
    prints "phasorbnk checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Cycles per control update, phase, bank count, bank index.
i 1 0 .0625 .25 0 2 0
i 1 0 .0625 2.5 0 2 0
i 1 0 .0625 -2.5 0 2 0
i 1 0 .0625 10.25 .25 2 1.75
i 1 0 .0625 1e20 .25 2 0
i 1 0 .0625 0 1 2 0
i 1 0 .0625 .25 -1 0 1
; Cycles per sample and initial phase, including partial blocks.
i 2 .125 .00390625 .25 0
i 2 .125 .00390625 2.5 .25
i 2 .125 .00390625 -2.5 .25
i 2 .125 .00390625 10.25 1
i 2 .125 .00390625 1e20 .25
i 2 .1256103515625 .0042724609375 2.5 .25
i 2 .1256103515625 .0042724609375 -2.5 .25
i 2 .1268310546875 .0001220703125 .25 1
; Initial, second, and third bank counts.
i 3 .25 .0625 2 4 4
i 3 .25 .0625 4 2 4
i 3 .25 .0625 2 2 2
i 99 .5 .01
e
</CsScore>
</CsoundSynthesizer>
