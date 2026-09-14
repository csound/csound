<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "errors/malformed-array.csd",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Unable to find opcode entry for '='"
    ]
  }
}
*/
; Try to provoke array parsing/compile errors (unmatched bracket / malformed array)
instr 1
  ; invalid array syntax in ORC/score region
  ; different Csound versions parse arrays differently; this should provoke a compile/parse warning/error
  iarr[] = 1,2,3
  a1 poscil 0.1, 440, 1
  outs a1, a1
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
