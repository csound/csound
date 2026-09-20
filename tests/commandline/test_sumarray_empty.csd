<CsTest>
description = "sumarray returns zero after an array becomes empty"
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
  iInput[] fillarray 3, 4
  trim_i iInput, 0
  iSum sumarray iInput
  if iSum != 0 then
    exitnow -1
  endif
  kInput[] fillarray 5, 6
  kCycle timeinstk
  kLength = (kCycle == 1 ? 2 : (kCycle == 2 ? 0 : 1))
  trim kInput, kLength
  kSum sumarray kInput
  kExpected = (kCycle == 1 ? 11 : (kCycle == 2 ? 0 : 5))
  if kSum != kExpected then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .046875
e
</CsScore>
</CsoundSynthesizer>
