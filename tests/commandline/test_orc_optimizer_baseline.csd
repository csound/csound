<CsTest>
description = "Optimizer-off preserves lowering, arithmetic and runtime warnings"
args = ["-n", "-d", "-m4", "-+orc_opt_passes=0", "--verbose=0x200"]
[expect]
exit = 0
output = ["ORC-OPT before", "ORC-OPT after", "ORC-OPT templates phase=resolved", "Division by zero", "optimizer-result=10"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr=48000
ksmps=16
nchnls=1
0dbfs=1
instr 1
 iresult = (2+3)*4/2
 prints "optimizer-result=%g\n", iresult
 kzero init 0
 kunused = 1/kzero
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
