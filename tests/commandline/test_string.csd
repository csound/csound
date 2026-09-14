<CsoundSynthesizer>
<CsInstruments>
/* Csound-test
{
  "description": "test string assignment and printing",
  "expect": {
    "exit": 0
  }
}
*/
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
