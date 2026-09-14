<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "reject unsupported pvsfilter input format",
  "expect": {
    "exit": "nonzero",
    "stderr": [
      "pvsfilter: signal format must be amp-phase or amp-freq."
    ]
  }
}
*/
sr = 8192
ksmps = 16
nchnls = 1
instr 1
  fInput pvsinit 64, 16, 64, 1, 2
  fOutput pvsfilter fInput, fInput, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
