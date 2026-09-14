<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "expected failure: GEN41 probability total overflows",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "Gen41: probability total must be finite"
    ]
  }
}
*/
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

giInvalid ftgen 1, 0, -8, -41, \
  10, 1e308, 20, 1e308, 30, 1e308
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
