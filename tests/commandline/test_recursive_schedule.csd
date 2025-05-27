<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

nchnls = 1
0dbfs = 1

instr 1
 print p4
 schedule 1, 0.001, 0, p4+1
endin

</CsInstruments>
<CsScore>
f 0 10
i1 0 0 0
</CsScore>
</CsoundSynthesizer>


