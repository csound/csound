<CsTest>
description = "swtget reports a performance error when a live source restarts with a different frame layout"

[expect]
exit = "nonzero"
stderr = ["swtget: SWT frame layout changed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aInput = 0.25
  layoutWave@global:swt swtanal aInput, p4, p5, 1, 2, 4, 0, 0, 1
endin

instr 2
  kCoeffs:k[] swtget layoutWave, 2
endin
</CsInstruments>
<CsScore>
i 1 0 0.05 64 64
i 2 0 0.2
i 1 0.06 0.05 32 32
e
</CsScore>
</CsoundSynthesizer>
