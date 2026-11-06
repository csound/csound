<CsTest>
description = "report orchestra line numbers after a macro expansion"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR in instr 1 (opcode table3.a) line 22:"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define FOO(x)#
prints "x=$x\n"#
instr 1
$FOO(3)
a1 table3 0, 999, 1
out a1
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
