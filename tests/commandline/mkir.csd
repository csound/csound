<CsoundSynthesizer>
<CsOptions>
 -U mkir sweep.wav -i rev.wav -o ir.wav
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "mkir.csd",
  "expect": {
    "exit": 0
  },
  "skip": "Manual impulse-response workflow: requires generated sweep.wav and rev.wav."
}
*/
</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
