<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject non-finite delay",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "vdelay3: invalid delay"
    ]
  }
}
*/
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
instr 1
  ain = .25
  kdelay = sqrt(-1)
  aout vdelay3 ain, kdelay, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
