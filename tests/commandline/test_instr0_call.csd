<CsTest>
description = "testing ability to call instr 0"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
prints "hello world!!!\n"
</CsInstruments>
<CsScore>
i0 0 0
</CsScore>
</CsoundSynthesizer>


