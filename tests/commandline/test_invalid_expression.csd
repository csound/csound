<CsoundSynthesizer>
<CsInstruments>
/* Csound-test
{
  "description": "test expression",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, opcode '##add' for expression with arg types cf not found",
      "syntax error, Unable to verify arg types for boolean expression '>'",
      "syntax error, conditional expression not valid"
    ]
  }
}
*/
instr 1
fsig pvsinit 1024
    kx init 1
    if kx > (3 + fsig) then
    endif
endin
</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
