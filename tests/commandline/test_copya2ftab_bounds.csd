<CsTest>
description = "copya2ftab clips to the table and follows the current source length"
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
  iTable ftgen 0, 0, -4, -2, -9, -9, -9, -9
  iInput[] fillarray 1, 2, 3
  copya2ftab iInput, iTable, 2
  if table:i(0, iTable) != -9 || table:i(2, iTable) != 1 || table:i(3, iTable) != 2 then
    exitnow -1
  endif
  kInput[] fillarray 4, 5, 6
  kCycle timeinstk
  if kCycle == 1 then
    copya2ftab kInput, iTable
  else
    trim kInput, 1
    kInput[0] = 7
    copya2ftab kInput, iTable, 0
  endif
  kFirst table 0, iTable
  kLast table 3, iTable
  kThird table 2, iTable
  if kFirst != (kCycle == 1 ? 4 : 7) || kThird != 6 || kLast != 2 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
e
</CsScore>
</CsoundSynthesizer>
