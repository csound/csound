<CsoundSynthesizer>
<CsOptions>
-odac -m128
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

//test for +=, -=, *=, /= on audio arrays

instr 1
 gaArr1[] init 2
 gaArr1[0] poscil .2, 400
 gaArr1[1] poscil .2, 500
 gaArr2[] init 2
 gaArr2[0] poscil .2, 600
 gaArr2[1] poscil .2, 700 
endin

instr 2
 aOutArr[] = gaArr1
 aOutArr += gaArr2
 outch p4, aOutArr[p4-1]
endin

instr 3
 aOutArr[] = gaArr1
 aOutArr *= gaArr2
 outch p4, aOutArr[p4-1]
endin

instr 4
 aOutArr[] = gaArr1
 aOutArr -= gaArr1 //results in silence
 outch p4, aOutArr[p4-1]
endin

instr 5
 aOutArr[] = gaArr1
 aOutArr /= (gaArr2+1)
 outch p4, aOutArr[p4-1]
endin

</CsInstruments>
<CsScore>
i 1 0 16
i 2 0 2 1
i 2 2 2 2
i 3 4 2 1
i 3 6 2 2
i 4 8 2 1
i 4 10 2 2
i 5 12 2 1
i 5 14 2 2
</CsScore>
</CsoundSynthesizer>
