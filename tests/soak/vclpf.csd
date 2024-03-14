<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1

a1 rand p4
af expon 20,p3,20000
a2 vclpf a1,af, 0.7
 out a2

endin

</CsInstruments>
<CsScore>
i1 0 5 0.1
</CsScore>
</CsoundSynthesizer>