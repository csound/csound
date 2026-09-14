<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "expected failure: GEN42 probability total overflows",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Gen42: probability total must be finite"
    ]
  }
}
*/
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

giInvalid ftgen 1, 0, -8, -42, \
  10, 12, 1e308, 20, 22, 1e308, 30, 32, 1e308
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
