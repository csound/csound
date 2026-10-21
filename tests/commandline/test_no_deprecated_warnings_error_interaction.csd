<CsTest>
description = "no-deprecated-warnings does not bypass error-deprecated"
args = ["-nd", "--error-deprecated", "--no-deprecated-warnings"]

[expect]
exit = "nonzero"
stderr = ["syntax error", "use sumarray instead"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

instr 1
  kValues[] fillarray 1, 2
  kTotal sumtab kValues
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
