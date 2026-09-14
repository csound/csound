<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject a selected zero grain4 pitch",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "granule_set: ipitch1 must be greater then zero"
    ]
  }
}
*/
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aout granule 1, 1, 1, 1, 0, 1, 1, 0, 0, 0.1, 0, 0, 0.002, 0, 50, 50, 0.5, 0, 1, 1, 1, 0
  out aout
endin
</CsInstruments>
<CsScore>
f 1 0 1024 7 1 1024 1
i 1 0 0.001
</CsScore>
</CsoundSynthesizer>
