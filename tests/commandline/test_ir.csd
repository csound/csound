<CsTest>
description = "test_ir.csd"
skip = "Manual impulse-response workflow: requires generated ir.wav and external fox.wav."

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

isw ftgen 1,0,0,-1,"ir.wav",0,0,1

instr 1
 asig diskin "fox.wav"
 arev ftconv asig, 1, 64
      out arev/2
endin


</CsInstruments>
<CsScore>
i1 0 5 
</CsScore>
</CsoundSynthesizer>
