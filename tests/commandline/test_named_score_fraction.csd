<CsTest>
description = "Named score events retain fractional p1 values"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 4
nchnls = 1
0dbfs = 1
instr Target
  if p1 != nstrnum("Target") + p4 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Target.25" 0 .008 .25
i "Target.75" .02 .008 .75
e
</CsScore>
</CsoundSynthesizer>
