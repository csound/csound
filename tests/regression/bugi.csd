<CsoundSynthesizer>

<CsInstruments>
instr 2
  gk2[][] init 3,4
  gk2[1][1] = 43
endin
  
instr 1
k0 init 42
k1[] fillarray 1,2,3
ii = 1
print i(k0)
print i(k1,1)
;print i(k1[ii])
print i(k1,k0)
print i(gk2,1,1)
;;print i(k1+1)
endin

</CsInstruments>
<CsScore>
i2 0 0.1
i1 1 0.1
e
</CsScore>

</CsoundSynthesizer>
