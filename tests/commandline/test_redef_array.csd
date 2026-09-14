<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test redef of OpcodeDef by an array",
  "expect": {
    "exit": 0
  }
}
*/
0dbfs = 1

partials@global:k[] init 7

instr 1

for val,i in partials do
partials[i] = 1
od
turnoff

endin
schedule(1,0,1)
</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


