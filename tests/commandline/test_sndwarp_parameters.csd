<CsTest>
description = "sndwarp and sndwarpst reject invalid overlap and window parameters"

[expect]
exit = "nonzero"
stderr = ["sndwarp: ioverlap must be a positive integer", "sndwarp: window size must be at least 2", "Wrong number of outputs in sndwarpst; must be 2 or 4"]
stderr_regex = ['25 errors in performance']
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
  aOut sndwarp 1, 1, 1, 1, 0, p5, p6, p4, 2, 0
endin
instr 2
  aLeft, aRight sndwarpst 1, 1, 1, 1, 0, p5, p6, p4, 2, 0
endin
instr 3
  aOut sndwarpst 1, 1, 1, 1, 0, 4, 0, 1, 2, 0
endin
</CsInstruments>
<CsScore>
f 1 0 16 -2 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
f 2 0 4 -2 1 1 1 1
i 1 0 .01 -1 4 0
i 1 0 .01 0 4 0
i 1 0 .01 0.5 4 0
i 1 0 .01 1.5 4 0
i 1 0 .01 2147483648 4 0
i 1 0 .01 1e+30 4 0
i 1 0 .01 1 0 0
i 1 0 .01 1 1 0
i 1 0 .01 1 1.5 0
i 1 0 .01 1 4 -1
i 1 0 .01 1 2147483648 0
i 1 0 .01 1 4 2147483648
i 2 0 .01 -1 4 0
i 2 0 .01 0 4 0
i 2 0 .01 0.5 4 0
i 2 0 .01 1.5 4 0
i 2 0 .01 2147483648 4 0
i 2 0 .01 1e+30 4 0
i 2 0 .01 1 0 0
i 2 0 .01 1 1 0
i 2 0 .01 1 1.5 0
i 2 0 .01 1 4 -1
i 2 0 .01 1 2147483648 0
i 2 0 .01 1 4 2147483648
i 3 0 .01
e
</CsScore>
</CsoundSynthesizer>
