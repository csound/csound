<CsTest>
description = "score seeds keep lowercase c inside a hexadecimal token"

[expect]
exit = 0
output_regex = ["seed from score 28\\r?\\nseed from score 28\\r?\\nseed from score 28"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
endin
</CsInstruments>
<CsScore>
y 28
y 0x1c
y 0x1C
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
