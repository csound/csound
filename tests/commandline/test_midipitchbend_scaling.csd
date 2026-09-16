<CsTest>
description = "midipitchbend scales normalized MIDI bends and preserves default and score values"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -F midipitchbend.mid
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
massign 1, 1
giMidiChecks init 0
giScoreChecks init 0
gkSeen[] init 5

instr 1
  iReference pchbend 0, 1
  iDefault init 99
  iOffset init 99
  iScaled init 99
  midipitchbend iDefault
  midipitchbend iOffset, 8
  midipitchbend iScaled, -2, 3
  if abs(iDefault-iReference) > 1e-6 || abs(iOffset-(8+iReference*119/127)) > 1e-6 || abs(iScaled-(-2+5*iReference)) > 1e-6 then
    prints "midipitchbend init scaling failed\n"
    exitnow -1
  endif
  giMidiChecks += 1

  kReference pchbend 0, 1
  kDefault init 99
  kOffset init 99
  kUnit init 99
  kScaled init 99
  kWide init 99
  kReverse init 99
  midipitchbend kDefault
  midipitchbend kOffset, 8
  midipitchbend kUnit, 0, 1
  midipitchbend kScaled, -2, 3
  midipitchbend kWide, 0, 127
  midipitchbend kReverse, 2, -2
  if abs(kDefault-kReference) > 1e-6 || abs(kOffset-(8+kReference*119/127)) > 1e-6 || abs(kUnit-kReference) > 1e-6 || abs(kScaled-(-2+5*kReference)) > 1e-6 || abs(kWide-127*kReference) > 1e-6 || abs(kReverse-(2-4*kReference)) > 1e-6 then
    printks "midipitchbend control scaling failed at bend %g\n", 0, kReference
    exitnowk -1
  endif

  if kReference == -1 then
    gkSeen[0] = 1
  elseif kReference == -.5 then
    gkSeen[1] = 1
  elseif kReference == 0 then
    gkSeen[2] = 1
  elseif kReference == .5 then
    gkSeen[3] = 1
  elseif kReference == 8191/8192 then
    gkSeen[4] = 1
  else
    prints "unexpected MIDI bend value\n"
    exitnowk -1
  endif
endin

instr 2
  iValue init 42
  kValue init 43
  midipitchbend iValue, -2, 3
  midipitchbend kValue, -2, 3
  if iValue != 42 then
    exitnow -1
  endif
  if kValue != 43 then
    exitnowk -1
  endif
  giScoreChecks += 1
endin

instr 99
  kSeen sumarray gkSeen
  if giMidiChecks != 6 || giScoreChecks != 1 || kSeen != 5 then
    printks "midipitchbend checks did not complete\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i2 0 .08
i99 .1 .001
</CsScore>
<CsFileB filename="midipitchbend.mid">
TVRoZAAAAAYAAAABAeBNVHJrAAAATADgAEAAkDxkCuAAAACQPWQBgD0ACuAAIACQPWQBgD0ACuAAQACQPWQBgD0ACuAAYACQPWQBgD0ACuB/fwCQPWQBgD0ACoA8AAD/LwA=
</CsFileB>
</CsoundSynthesizer>
