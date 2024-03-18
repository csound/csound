<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1

kin1[] fillarray 1,2,3,4
kin2[] fillarray 5,6,7,8

kInt[] interleave kin1, kin2

printf "inputs: \n%d %d %d %d \n%d %d %d %d\n", 1,
         kin1[0], kin1[1], kin1[2], kin1[3],
         kin2[0], kin2[1], kin2[2], kin2[3]

printf "interleaved:\n%d %d %d %d %d %d %d %d\n", 1,
         kInt[0], kInt[1], kInt[2], kInt[3],
         kInt[4], kInt[5], kInt[6], kInt[7]
endin

</CsInstruments>
<CsScore>
i1 0 1	
e
</CsScore>
</CsoundSynthesizer>
