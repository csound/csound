<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

kArr[][] init 2, 4

kArr[0][0] = 3
kArr[0][1] = 5
kArr[0][2] = 7 
kArr[0][3] = 9 
kArr[1][0] = kArr[0][0] * kArr[0][0]
kArr[1][1] = kArr[0][1] * kArr[0][1]
kArr[1][2] = kArr[0][2] * kArr[0][2]
kArr[1][3] = kArr[0][3] * kArr[0][3]

printks "kArr[0][0] = %f\nkArr[0][1] = %f\nkArr[0][2] = %f\nkArr[0][3] = %f\n", .1, kArr[0][0], kArr[0][1], kArr[0][2], kArr[0][3]
printks "kArr[1][0] = %f\nkArr[1][1] = %f\nkArr[1][2] = %f\nkArr[2][3] = %f\n", .1, kArr[1][0], kArr[1][1], kArr[1][2], kArr[1][3]

turnoff

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
