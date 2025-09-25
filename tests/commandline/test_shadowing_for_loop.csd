<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
for partials,count in [1,2.78,5.18,8.16,11.66,15.64,19.99] do
 S1 = sprintf("partial %d\n",count+1)
 prints S1
od
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

