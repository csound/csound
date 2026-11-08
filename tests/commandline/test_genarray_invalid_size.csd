<CsTest>
description = "genarray rejects oversized lengths, zero increments, and matrix outputs"
[expect]
exit = "nonzero"
stderr = ["genarray: sequence length out of range", "genarray: increment must be non-zero", "genarray: output must be one-dimensional"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr InitSize
  iValues[] genarray 0, 1e30
endin
instr PerfSize
  kEnd init p4
  kValues[] genarray 0, kEnd, p5
endin
instr Matrix
  iValues[][] init 1, 1
  iValues genarray 0, 3
endin
</CsInstruments>
<CsScore>
i "InitSize" 0 .015625
i "PerfSize" .03125 .015625 1e30 1
i "PerfSize" .0625 .015625 1 0
i "Matrix" .09375 .015625
e
</CsScore>
</CsoundSynthesizer>
