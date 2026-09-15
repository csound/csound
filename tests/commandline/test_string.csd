<CsTest>
description = "test string assignment and printing"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
instr 1
Sres = "TEST"
puts Sres, 1
turnoff
endin

</CsInstruments>
<CsScore>
i1 0 .5
</CsScore>
</CsoundSynthesizer>
