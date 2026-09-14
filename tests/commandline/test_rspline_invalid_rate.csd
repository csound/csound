<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject invalid rspline rates",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "rspline: rates must be finite and non-negative"
    ]
  }
}
*/
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aresult rspline 0, 1, -1, 1
  out aresult
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
