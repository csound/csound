<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "testing clean compilestr fail",
  "expect": {
    "exit": 0
  }
}
*/
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


