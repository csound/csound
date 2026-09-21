<CsTest>
description = "ftprint rejects invalid tables, ranges, steps, and column counts"
[expect]
exit = "nonzero"
stderr = ["ftprint: end index out of range", "ftprint: step must be a positive integer", "Invalid ftable no.", "ftprint: start index out of range", "ftprint: invalid column count"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
giTable ftgen 1, 0, -4, -2, 1, 2, 3, 4
instr 1
  ftprint p4, p5, p6, p7, p8, p9
endin
</CsInstruments>
<CsScore>
; Init-time bounds, performance-time zero step, and a missing table.
i 1 0 .015625 1 1 0 8 1 0
i 1 .03125 .015625 1 -1 0 0 0 0
i 1 .0625 .015625 999999 1 0 0 1 0
i 1 .09375 .015625 1 1 -10 0 1 0
i 1 .125 .015625 1 1 0 0 1 -1
e
</CsScore>
</CsoundSynthesizer>
