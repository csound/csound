<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test Arr:a[] = [sig:a]",
  "expect": {
    "exit": 0
  }
}
*/
0dbfs = 1
instr 1
 Arr:a[] = [oscili(0.5,440)]
 out Arr
endin
</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>