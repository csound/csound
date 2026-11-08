<CsTest>
description = "explicit-base array log follows input lengths and supports input reuse"

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
gkChecks init 0

instr 1
  iInput[] fillarray .25, 1, 4, 16
  iLogs[] log iInput, p4
  iIndex = 0
  while iIndex < 4 do
    if abs(iLogs[iIndex] - log(iInput[iIndex])/log(p4)) > .00001 then
      prints "incorrect init-time logarithm\n"
      exitnow -1
    endif
    iIndex += 1
  od

  kInput[] init 4
  kReuse[] init 4
  kCycle timeinstk
  kLength = (kCycle == 2 ? 1 : (kCycle == 3 ? 0 : 4))
  trim kInput, kLength
  trim kReuse, kLength
  kIndex = 0
  while kIndex < kLength do
    kInput[kIndex] = pow(p4, kIndex-kCycle)
    kReuse[kIndex] = kInput[kIndex]
    kIndex += 1
  od
  kLogs[] log kInput, p4
  kReuse log kReuse, p4
  kNatural[] log kInput, 0
  kSize lenarray kLogs
  kReuseSize lenarray kReuse
  kNaturalSize lenarray kNatural
  if kSize != kLength || kReuseSize != kLength || kNaturalSize != kLength then
    printks "incorrect logarithm array length\n", 0
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    if abs(kLogs[kIndex]-(kIndex-kCycle)) > .00001 || abs(kReuse[kIndex]-kLogs[kIndex]) > .00001 || abs(kNatural[kIndex]-log(kInput[kIndex])) > .00001 then
      printks "incorrect control-time logarithm\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 4 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 3 then
    prints "logarithm checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .1 2
i1 .2 .1 10
i1 .4 .1 .5
i99 .6 .01
</CsScore>
</CsoundSynthesizer>
