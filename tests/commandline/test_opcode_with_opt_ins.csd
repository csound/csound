<CsTest>
description = "test opcode with opt ins only"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
icps veloc
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>

