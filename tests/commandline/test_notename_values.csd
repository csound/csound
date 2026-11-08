<CsTest>
description = "note converters preserve accidentals, quarter tones, cents and A4 tuning"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
A4 = 442
instr 1
  Snote strget p4
  iMidi ntom Snote
  iFreq ntof Snote
  iExpected = 442 * 2 ^ ((p5 - 69) / 12)
  if abs(iMidi - p5) > .0001 || abs(iFreq / iExpected - 1) > .0001 then
    exitnow -1
  endif
  kMidi ntom Snote
  kFreq ntof Snote
  if abs(kMidi - p5) > .0001 || abs(kFreq / iExpected - 1) > .0001 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 "4A" 69
i 1 0 .01 "0Cb" 11
i 1 0 .01 "9B#" 132
i 1 0 .01 "4C+" 60.5
i 1 0 .01 "4D-" 61.5
i 1 0 .01 "4F#+5" 66.05
i 1 0 .01 "4Db-10" 60.9
i 1 0 .01 "4C#+99" 61.99
e
</CsScore>
</CsoundSynthesizer>
