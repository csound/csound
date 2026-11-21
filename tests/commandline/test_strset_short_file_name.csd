<CsTest>
description = "A strset string or string p-field that fits a file-name buffer is used whole, and a number with neither names its base file"

[expect]
exit = 0
output = ["checked names that fit"]
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

opcode Check(iOk, SWhat):void
  if iOk == 0 then
    prints "failed: %s\n", SWhat
    exitnow -1
  endif
endop

instr 1
  ; The 1024-byte buffers: sndinfo and dumpf, by a number that is a strset string.
  strset 2, "fox.wav"
  Check(filelen(2) == filelen("fox.wav") ? 1 : 0, "filelen by number")
  Check(filevalid(2) == 1 ? 1 : 0, "filevalid by number")
  ; GEN44's 80-byte buffer, the smallest, by a name that fits it.
  strset 3, "gen44_matrix_valid.txt"
  iTable ftgen 0, 0, 0, 44, 3
  Check(iTable > 0 ? 1 : 0, "GEN44 by number")
  ; MAXNAME: atsinfo, by number and by name, through the same reader.
  strset 4, "ats-instance.ats"
  Check(atsinfo(4, 0) == atsinfo("ats-instance.ats", 0) ? 1 : 0, "atsinfo by number")
  ; A strset number and a p-field string through readfi, the buffer behind readf.
  strset 5, "readf_lines.txt"
  S1, i1 readfi 5
  S2, i2 readfi "readf_lines.txt"
  Check(strcmp(S1, S2) == 0 && i1 == i2 ? 1 : 0, "readfi by number")
endin

instr 2
  ; The output names are the buffer's, exactly: this one is read back below.
  strset 6, "strset_short_name_fit.dat"
  dumpk k(0.25), 6, 8, 0
  ; A number with no strset string: dumpk.7, no more and no less.
  dumpk k(0.5), 7, 8, 0
endin

instr 3
  S1, i1 readfi p4
  S2, i2 readfi "readf_lines.txt"
  Check(strcmp(S1, S2) == 0 && i1 == i2 ? 1 : 0, "readfi by string p-field")
  ; A quote inside a string p-field ends the name.
  S3, i3 readfi p6
  Check(strcmp(S3, S2) == 0 && i3 == i2 ? 1 : 0, "a quote ends a string p-field's name")
  Check(filevalid("strset_short_name_fit.dat") == 1 ? 1 : 0, "a strset string names the file dumpk wrote")
  Check(filevalid("dumpk.7") == 1 ? 1 : 0, "a number with no strset string names a base file")
  iTable ftgen 0, 0, 8, 10, 1
  ftsave p5, 0, iTable, iTable
  Check(filevalid(p5) == 1 ? 1 : 0, "ftsave by string p-field")
  prints "checked names that fit\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0 0.05
i 3 0.1 0.05 "readf_lines.txt" "strset_short_name_fit.tab" "readf_lines.txt\"tail"
</CsScore>
</CsoundSynthesizer>
