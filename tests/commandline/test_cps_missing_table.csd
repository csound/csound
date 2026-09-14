<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 iResult cpsxpch 1.04, -100, 2, 100
endin
instr 2
 iResult cps2pch 1.04, -100
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 0 .001
e
</CsScore>
</CsoundSynthesizer>
