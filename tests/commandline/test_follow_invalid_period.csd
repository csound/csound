<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject invalid follow period",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "follow: invalid period"
    ]
  }
}
*/
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  iperiod = (p4 == 0 ? sqrt:i(-1) : p4)
  ain = 0
  aout follow ain, iperiod
  out aout
endin
</CsInstruments>
<CsScore>
i 1 0 .001 0
i 1 0 .001 50000
i 1 0 .001 -50000
</CsScore>
</CsoundSynthesizer>
