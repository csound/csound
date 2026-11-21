<CsTest>
description = "k-rate filevalid with a string p-field reads the name at init and works at performance, where the note's strings are gone"

[expect]
exit = 0
output = ["filevalid found the file: 1", "filevalid did not find the other: 0"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  k_ filevalid p4
  printks "filevalid found the file: %d\n", 0, k_
  turnoff
endin

instr 2
  k_ filevalid p4
  printks "filevalid did not find the other: %d\n", 0, k_
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 "test_filevalid_k_pfield.csd"
i 2 0.05 0.01 "no_such_file_for_filevalid"
</CsScore>
</CsoundSynthesizer>
