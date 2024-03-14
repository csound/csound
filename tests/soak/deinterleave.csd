<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1

kInt[] fillarray 1,2,3,4,5,6,7,8

kout1[],kout2[] deinterleave kInt

printf "input: \n%d %d %d %d %d %d %d %d\n", 1,
         kInt[0], kInt[1], kInt[2], kInt[3],
         kInt[4], kInt[5], kInt[6], kInt[7]


printf "de-interleaved:\n%d %d %d %d \n%d %d %d %d\n", 1,
         kout1[0], kout1[1], kout1[2], kout1[3],
         kout2[0], kout2[1], kout2[2], kout2[3]
endin

</CsInstruments>
<CsScore>
i1 0 1	
e
</CsScore>
</CsoundSynthesizer>
