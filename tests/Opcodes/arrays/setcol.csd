<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
instr 1
 kcnt init 0
 kArr[] init 3,3
 kVec[] fillarray  0,1,2
 while kcnt < 3 do
  kArr setcol kVec,kcnt
  printf "column %d: %d %d %d\n",kcnt+1,kcnt,kArr[0][kcnt],kArr[1][kcnt],kArr[2][kcnt] 
  kcnt += 1
 od
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
