<CsoundSynthesizer>
<CsOptions>
-o rev.wav -W -f
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "mkrev.csd",
  "expect": {
    "exit": 0
  },
  "skip": "Manual impulse-response workflow: requires generated sweep.wav."
}
*/
0dbfs=1

instr 1
  asig diskin "sweep.wav"
  arev reverb asig, p3/2
  out arev
endin

</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
