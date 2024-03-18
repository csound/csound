<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
instr 1
 kcnt init 0
 kArr[] init 3,3
 kArr[] fillarray  0,1,2,0,1,2,0,1,2
 while kcnt < 3 do
  kVec[] getcol kArr,kcnt
  printf "column %d: %d %d %d\n",kcnt+1,kcnt,kVec[0],kVec[1],kVec[2] 
  kcnt += 1
 od
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
