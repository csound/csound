<CsTest>
description = "tabmorph wraps indices in the array and variadic forms"
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
giA ftgen 0, 0, 4, -2, 0, 10, 20, 30
giB ftgen 0, 0, 4, -2, 100, 110, 120, 130
instr 1
  iTables[] fillarray giA, giB
  kCycle timeinstk
  kIndex = (kCycle == 1 ? -1 : (kCycle == 2 ? 4294967296 : 5))
  kExpected = (kCycle == 1 ? 80 : (kCycle == 2 ? 50 : 60))
  aIndex = kIndex / 4
  aWeight = .5
  aChoice = -.5
  kV tabmorph kIndex, .5, -.5, -.5, giA, giB
  kA tabmorph kIndex, .5, -.5, -.5, iTables
  kVi tabmorphi kIndex, .5, -.5, -.5, giA, giB
  kAi tabmorphi kIndex, .5, -.5, -.5, iTables
  aV tabmorpha aIndex, aWeight, aChoice, aChoice, giA, giB
  aA tabmorpha aIndex, aWeight, aChoice, aChoice, iTables
  aVk tabmorphak aIndex, .5, -.5, -.5, giA, giB
  aAk tabmorphak aIndex, .5, -.5, -.5, iTables
  kAV downsamp aV
  kAA downsamp aA
  kAVk downsamp aVk
  kAAk downsamp aAk
  if kV != kExpected || kA != kExpected || kVi != kExpected || kAi != kExpected || kAV != kExpected || kAA != kExpected || kAVk != kExpected || kAAk != kExpected then
    exitnowk -1
  endif
endin
instr Limit
  iTables[] init 1998
  iIndex = 0
  while iIndex < 1998 do
    iTables[iIndex] = giB
    iIndex += 1
  od
  kValue tabmorph 0, 0, 1997.5, 0, iTables
  if kValue != 100 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .046875
i "Limit" .0625 .015625
e
</CsScore>
</CsoundSynthesizer>
