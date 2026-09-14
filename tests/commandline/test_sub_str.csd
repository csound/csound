<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


/* Csound-test
{
  "description": "test raw string embedded in raw string",
  "expect": {
    "exit": 0
  }
}
*/
instr 1
ires compilestr {{
prints {{test string
}}
}}
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

