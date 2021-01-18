<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
0dbfs = 1
nchnls = 2
ksmps = 32

instr 1
aEnv expseg 0.0001, .2, 1, .2, .5, .2, .7, .2, 0.0001
a1 oscili aEnv, 400
outs a1, a1
endin

instr 2
kTrig metro 1
aEnv trigExpseg kTrig, 0.0001, .2, 1, .2, .5, .2, .7, .2, 0.0001
a1 oscili aEnv, 400
outs a1, a1
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 3 8
</CsScore>
</CsoundSynthesizer>
