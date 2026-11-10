<CsTest>
description = "ftsave and ftload take a file name from a number or a string p-field, and so do ftsavek and ftloadk at performance"

[expect]
exit = 0
output = ["checked ftsave and ftload", "checked ftsavek and ftloadk"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

giA ftgen 1, 0, 8, -7, 0.25, 8, 0.25
giB ftgen 2, 0, 8, -7, 0.5, 8, 0.5
giC ftgen 3, 0, 8, 7, 0, 8, 0
giD ftgen 4, 0, 8, 7, 0, 8, 0

opcode Check(iOk, SWhat):void
  if iOk == 0 then
    prints "failed: %s\n", SWhat
    exitnow -1
  endif
endop

instr 1
  ; A number that is a strset string, and ftsave's whole list of tables.
  strset 5, "ftsave_by_number.tab"
  ftsave 5, 0, 1, 2
  Check(filevalid(5) == 1 ? 1 : 0, "ftsave by strset number")
  ftload 5, 0, 3, 4
  Check(table(0, 3) == 0.25 && table(0, 4) == 0.5 ? 1 : 0, "ftsave saved every table, ftload by number loaded them")

  ; A number with no strset string: ftsave.7, and ftload finds it.
  ftsave 7, 0, 2
  Check(filevalid("ftsave.7") == 1 ? 1 : 0, "ftsave by number with no strset string")
  ftload 7, 0, 4
  Check(table(0, 4) == 0.5 ? 1 : 0, "ftload by number with no strset string")
  prints "checked ftsave and ftload\n"
endin

instr 2
  ; A string p-field, and ftsave's whole list of tables.
  ftsave p4, 0, 2, 1
  ftload p4, 0, 3, 4
  Check(table(0, 3) == 0.5 && table(0, 4) == 0.25 ? 1 : 0, "ftsave by string p-field saved every table")
endin

instr 3
  ; ftsavek and ftloadk, whose names cannot be read at performance: a string
  ; p-field, a number with no strset string, a strset number.
  strset 9, "ftsavek_by_number.tab"
  kCycle timeinstk
  if kCycle == 1 then
    ftsavek p4, 1, 0, 1, 2
    ftsavek 11, 1, 0, 2
    ftsavek 9, 1, 0, 1
  endif
  if kCycle == 2 then
    ftloadk p4, 1, 0, 3, 4
    ftloadk 11, 1, 0, 3
    ftloadk 9, 1, 0, 4
  endif
endin
instr 4
  Check(filevalid(p4) == 1 ? 1 : 0, "ftsavek by string p-field")
  Check(filevalid("ftsave.11") == 1 ? 1 : 0, "ftsavek by number with no strset string")
  Check(filevalid("ftsavek_by_number.tab") == 1 ? 1 : 0, "ftsavek by strset number")
  Check(table(0, 3) == 0.5 ? 1 : 0, "ftloadk by number with no strset string")
  Check(table(0, 4) == 0.25 ? 1 : 0, "ftloadk by strset number")
  prints "checked ftsavek and ftloadk\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0 0.05 "ftsave_by_pfield.tab"
i 3 0.1 0.05 "ftsavek_by_pfield.tab"
i 4 0.2 0.05 "ftsavek_by_pfield.tab"
</CsScore>
</CsoundSynthesizer>
