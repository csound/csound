<CsTest>
description = "mkrev.csd"
skip = "Manual impulse-response workflow: requires generated sweep.wav."

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-o rev.wav -W -f
</CsOptions>
<CsInstruments>
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
