<CsTest>
description = "a UDO with inputs, no outputs and an empty body does nothing, and compiles"

[expect]
exit = 0
output = ["pass empty bodies"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 1

opcode NothingWithInput, 0, i
endop

opcode NothingNew(value:i):void
endop

instr 1
NothingWithInput 3
NothingNew(4)
prints "pass empty bodies\n"
turnoff
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
