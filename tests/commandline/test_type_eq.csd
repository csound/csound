<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test type equality operator",
  "expect": {
    "exit": 0
  }
}
*/
0dbfs = 1

instr 1
k:i init 0
j:i init 0
prints typeof(k) == typeof(j) ? "types match\n" : "types do not match\n" 
endin



</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>


