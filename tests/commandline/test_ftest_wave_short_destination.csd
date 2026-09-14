<CsoundSynthesizer>
<CsOptions>
-d -m0 -n
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject an undersized wave table without crashing",
  "expect": {
    "exit": 0
  }
}
*/
sr=48000
ksmps=32
nchnls=1
instr 1
endin
</CsInstruments>
<CsScore>
f 2 0 64 10 1
f 1 0 32 "wave" 2 1 0
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
