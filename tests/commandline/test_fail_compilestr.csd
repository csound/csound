<CsTest>
description = "testing clean compilestr fail"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>

ires = compilestr({{
instr 1
  nonsense()
endin
}})

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>


