<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "syntax error on redefinition of local var by global var in same context",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, global variable num:k cannot shadow local variable num:i"
    ]
  }
}
*/
instr 1
num:i init 0 
num@global:k init 2 // error:  global variable num:k cannot shadow local variable num:i
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>