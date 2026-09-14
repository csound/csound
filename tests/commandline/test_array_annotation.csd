<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "testing array type annotation for opcodes",
  "expect": {
    "exit": 0
  }
}
*/
instr 1
arr:k[] = genarray:k[](0, 10)
printarray(arr)
turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


