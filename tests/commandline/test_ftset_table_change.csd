<CsTest>
description = "ftset follows table changes at performance time"
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
gkCycles init 0

instr ChangeTable
  iFirst ftgen 0, 0, -4, -2, 0
  iSecond ftgen 0, 0, -8, -2, 0
  kCycle timeinstk

  ; Three control cycles: first table = 10, second = 20, first = 30.
  ; Different lengths also check that ftset uses the new table's bounds.
  kTable = (kCycle == 2 ? iSecond : iFirst)
  ftset kTable, kCycle * 10

  ; Check both tables each time, including the one that was not selected.
  kExpectedFirst = (kCycle < 3 ? 10 : 30)
  kExpectedSecond = (kCycle == 1 ? 0 : 20)
  kIndex = 0
  while kIndex < 4 do
    kActual table kIndex, iFirst
    if kActual != kExpectedFirst then
      printks "ftset first table: cycle %g index %g, got %g, expected %g\n", 0, kCycle, kIndex, kActual, kExpectedFirst
      exitnowk -1
    endif
    kIndex += 1
  od
  kIndex = 0
  while kIndex < 8 do
    kActual table kIndex, iSecond
    if kActual != kExpectedSecond then
      printks "ftset second table: cycle %g index %g, got %g, expected %g\n", 0, kCycle, kIndex, kActual, kExpectedSecond
      exitnowk -1
    endif
    kIndex += 1
  od
  gkCycles = kCycle
endin

instr CheckDone
  if i(gkCycles) != 3 then
    prints "ftset table-change test did not run all three cycles\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "ChangeTable" 0 .046875
i "CheckDone" .0625 .015625
</CsScore>
</CsoundSynthesizer>
