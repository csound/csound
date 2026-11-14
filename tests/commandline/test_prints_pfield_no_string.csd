<CsTest>
description = "prints refuses a string p-field code with no string behind it, as it does when given values"

[expect]
exit = "nonzero"
stderr_regex = [
  'INIT ERROR in instr 2 .*null string',
  'INIT ERROR in instr 3 .*null string',
]
output_excludes = ["(null)", "|after"]
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

instr 1
  ; The code of a string p-field, copied to a note that has no strings.
  iCode = p4
  schedule 2, 0, .1, iCode
  schedule 3, .1, .1, iCode
endin

instr 2
  prints p4
  prints "|after\n"
endin

instr 3
  prints p4, 1
  prints "|after\n"
endin

</CsInstruments>
<CsScore>
i 1 0 .3 "text"
e
</CsScore>
</CsoundSynthesizer>
