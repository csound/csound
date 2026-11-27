<CsTest>
description = "swtset reports a layout error when a source restarts with a different layout at equal capacity"

[expect]
exit = "nonzero"
stderr = ["swtset: SWT frame layout changed"]
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
  aInput = 0.5
  setWave@global:swt swtanal aInput, p4, p4, 1, 2, p5, 0, 0, 1
endin

instr 2
  kCoeffs:k[] init 192
  editedWave@global:swt swtset setWave, kCoeffs, -1
  aOutput swtsynth editedWave
endin
</CsInstruments>
<CsScore>
i 1 0 0.05 64 2
i 2 0 0.2
i 1 0.06 0.05 48 3
e
</CsScore>
</CsoundSynthesizer>
