<CsTest>
description = "Array int follows control-rate values and lengths"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
gkChecks init 0

instr 1
  iInput[] fillarray 1.75, -1.75, 0.75, -0.75
  iResult[] int iInput
  if iResult[0] != 1 || iResult[1] != -1 || iResult[2] != 0 || iResult[3] != 0 then
    prints "int: incorrect init-rate truncation\n"
    exitnow -1
  endif

  kInput[] init 4
  kReuse[] init 4
  kCycle timeinstk
  kLength = (kCycle == 2 ? 2 : (kCycle == 3 ? 0 : 4))
  trim kInput, kLength
  trim kReuse, kLength
  kJ = 0
  while kJ < kLength do
    kValue = (kJ % 2 == 0 ? 1 : -1) * (kCycle + kJ + 0.75)
    kInput[kJ] = kValue
    kReuse[kJ] = kValue
    kJ += 1
  od
  kResult[] int kInput
  kReuse int kReuse
  kResultLength lenarray kResult
  kReuseLength lenarray kReuse
  if kResultLength != kLength || kReuseLength != kLength then
    printks "int: output length did not follow input\n", 0
    exitnowk -1
  endif
  kJ = 0
  while kJ < kLength do
    kExpected = int(kInput[kJ])
    if kResult[kJ] != kExpected || kReuse[kJ] != kExpected then
      printks "int cycle %g index %g: expected %g, got %g and %g\n", 0, kCycle, kJ, kExpected, kResult[kJ], kReuse[kJ]
      exitnowk -1
    endif
    kJ += 1
  od
  gkChecks += 1
  if kCycle == 4 then
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "int array checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i99 .02 .001
</CsScore>
</CsoundSynthesizer>
