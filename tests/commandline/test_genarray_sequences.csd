<CsTest>
description = "genarray preserves descending sequences and resizes control-rate results"
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
instr 1
  iDescending[] genarray 3, -1, -.5
  if lenarray(iDescending) != 9 || iDescending[8] != -1 then
    exitnow -1
  endif
  iFractional[] genarray 0, 1, .1
  if lenarray(iFractional) != 11 then
    exitnow -1
  endif
  kCycle timeinstk
  kEnd = (kCycle == 1 ? 2 : (kCycle == 2 ? 4 : 0))
  kValues[] genarray 0, kEnd
  kLength lenarray kValues
  if kLength != kEnd + 1 || kValues[kLength - 1] != kEnd then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .046875
e
</CsScore>
</CsoundSynthesizer>
