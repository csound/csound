<CsTest>
description = "Automatic-size GEN02 tables have complete metadata, normalization and guard points"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

giFixed ftgen 1, 0, 4, 2, 1, 2, 3, 4
giAuto ftgen 2, 0, 0, 2, 1, 2, 3, 4
giOdd ftgen 3, 0, 0, -2, 1, 2, 3, 4, 5
giOne ftgen 4, 0, 0, -2, 7

instr 1
  if ftlen(giAuto) != 4 || ftchnls(giAuto) != 1 || ftlen(giOdd) != 5 || ftchnls(giOdd) != 1 || ftlen(giOne) != 1 then
    exitnow -1
  endif
  iIndex = 0
  while iIndex < 4 do
    iActual tablei iIndex, giAuto
    iExpected tablei iIndex, giFixed
    if iActual != iExpected then
      exitnow -1
    endif
    iIndex += 0.5
  od
  ; The last value stays in the table; the guard wraps back to the first.
  iOddLast tab_i 4, giOdd
  iOddEdge tablei 4.5, giOdd
  iOneEdge tablei 0.5, giOne
  if iOddLast != 5 || iOddEdge != 3 || iOneEdge != 7 then
    exitnow -1
  endif

  ; oscil uses the phase metadata that table reads do not need.
  kActual oscil 1, 0, giAuto, 0.25
  kExpected oscil 1, 0, giFixed, 0.25
  if kActual != kExpected then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
