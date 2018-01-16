<CsoundSynthesizer>

<CsInstruments>
 gkArr[] fillarray 1, 2, 3, 4, 5
instr 1
 iArr[] fillarray 6, 7, 8, 9, 0
;; iArr = gkArr ;copy kArr at i-time to iArr
 gkArr = iArr ;copy iArr during performance to kArr
endin

</CsInstruments>

<CsScore>
i1 0 1
</CsScore>

</CsoundSynthesizer>
