<CsoundSynthesizer>
<CsInstruments>
/* Csound-test
{
  "description": "test expression",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "syntax error, unable to find ternary operator for types 'b ? i : f'"
    ]
  }
}
*/
instr 1
fsig pvsinit 1024
ivar = 45
kval = (p3 > 0) ? ivar : fsig 

endin
</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
