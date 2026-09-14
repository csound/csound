<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject invalid fold increment",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "fold: increment must be finite and >= 1"
    ]
  }
}
*/
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  asignal oscili .5, 440
  aresult fold asignal, 0
  out aresult
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
