<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject non-finite distort filter frequency",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "distort: half-power frequency must be finite"
    ]
  }
}
*/
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

gishape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1

instr 1
  ihp = sqrt(-1)
  asignal init 0
  aresult distort asignal, 1, gishape, ihp
  out aresult
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
