<CsoundSynthesizer>

<CsOptions>
-n
</CsOptions>
<CsInstruments>
gkArray[][] init   2,3
gkArray     fillarray  1,2,3,7,6,5

instr 1
k1 changed2 gkArray
if k1==1 then
    printks "An element in the array changed", 0
endif
endin

instr 2; change value of channel 'step1'
    gkArray[1][0] = 3
endin

</CsInstruments>
<CsScore>
i1 0 100
i2 4 .1
</CsScore>

</CsoundSynthesizer>
