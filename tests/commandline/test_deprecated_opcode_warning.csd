<CsTest>
description = "deprecated opcode reports a replacement warning"
args = ["-nd"]

[expect]
exit = 0
stderr_regex = ["opcode sumtab is deprecated; use sumarray instead, line \\d+"]
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
