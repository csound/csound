<CsTest>
description = "ftset reversed clear range and first built-in table lookup"
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

instr ReversedClear
  ; Clearing an empty range must not pass a negative length to memset.
  iTable ftgen 0, 0, -8, -2, 1, 2, 3, 4, 5, 6, 7, 8
  ftset iTable, 0, 6, 2
  kValue init 0
  ftset iTable, kValue, 6, 2
  kIndex = 0
  while kIndex < 8 do
    kActual table kIndex, iTable
    if kActual != kIndex + 1 then
      printks "ftset changed an entry outside its range\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
endin

instr BuiltinTable
  ; -1 used to match the empty cache sentinel and skip the first lookup.
  kValue init 9
  ftset -1, kValue, 0, 1
  kActual table 0, -1
  if kActual != 9 then
    printks "ftset did not write to the built-in table\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i "ReversedClear" 0 .015625
i "BuiltinTable" .015625 .015625
e
</CsScore>
</CsoundSynthesizer>
