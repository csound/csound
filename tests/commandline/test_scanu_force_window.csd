<CsTest>
description = "scanu and scanu2 keep force windows local to each network"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkDone init 0

instr 1
  iSize = p4
  iZero ftgen 0, 0, -iSize, -2, 0
  iMass ftgen 0, 0, -iSize, -7, 1, iSize, 1
  iMatrix ftgen 0, 0, -(iSize*iSize), -2, 0
  aDrive = .01
  if p5 == 0 then
    scanu iZero, iSize/sr, iZero, iMass, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, p6
  else
    scanu2 iZero, iSize/sr, iZero, iMass, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, p6
  endif
  kPosition[], kVelocity[] scanmap p6
  kCycle init 0
  kCycle += 1
  ; With no internal forces, every update adds just the windowed input.
  ; The first update happens after the input ring has filled completely.
  kUpdates = int((kCycle-1)/iSize)
  kIndex = 0
  while kIndex < iSize do
    kWindow = 0
    if iSize > 1 && kIndex < iSize-1 then
      kWindow = sqrt(sin($M_PI*kIndex/(iSize-1)))
    endif
    kExpected = .01*kUpdates*kWindow
    if !(abs(kVelocity[kIndex]-kExpected) < .000001) then
      printks "scanu variant %g size %g mass %g: got %g, expected %g\n", 0, p5, iSize, kIndex, kVelocity[kIndex], kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 3*iSize+1 then
    gkDone += 1
    turnoff
  endif
endin

; A negative ID writes the state to the table with that absolute number.
instr 2
  iZero ftgen 0, 0, 8, -2, 0
  iOne ftgen 0, 0, 8, -7, 1, 8, 1
  iMatrix ftgen 0, 0, 64, -2, 0
  iOutput ftgen 0, 0, -p5, -2, 0
  aDrive = 0
  if p4 == 0 then
    scanu iZero, 1/sr, iOne, iOne, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, -iOutput
  else
    scanu2 iZero, 1/sr, iOne, iOne, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, -iOutput
  endif
  kCycle init 0
  kCycle += 1
  kIndex = 0
  while kIndex < p5 do
    kValue table kIndex, iOutput
    kExpected = (kIndex < 8 ? max(0, kCycle-2) : 0)
    if !(abs(kValue-kExpected) < .000001) then
      printks "scanu table output mass %g: got %g, expected %g\n", 0, kIndex, kValue, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 8 then
    gkDone += 1
    turnoff
  endif
endin

instr 99
  if i(gkDone) != 24 then
    prints "scanu window checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Mixed sizes coexist, then repeat on reused instrument instances.
i 1 0 .1 8 0 1
i 1 0 .1 32 1 2
i 1 0 .1 4 0 3
i 1 0 .1 7 1 4
i 1 0 .1 1 0 5
i 1 0 .1 8 1 6
i 1 0 .1 32 0 7
i 1 0 .1 4 1 8
i 1 0 .1 7 0 9
i 1 0 .1 1 1 10
i 1 .2 .1 32 0 1
i 1 .2 .1 4 1 2
i 1 .2 .1 7 0 3
i 1 .2 .1 1 1 4
i 1 .2 .1 8 0 5
i 1 .2 .1 32 1 6
i 1 .2 .1 4 0 7
i 1 .2 .1 7 1 8
i 1 .2 .1 1 0 9
i 1 .2 .1 8 1 10
i 2 0 .1 0 8
i 2 0 .1 1 8
i 2 0 .1 0 16
i 2 0 .1 1 16
i 99 .4 .001
e
</CsScore>
</CsoundSynthesizer>
