<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

kArr[][] init 2, 2

kArr[0][0] = 3
kArr[0][1] = 5
kArr[1][0] = kArr[0][0] * kArr[0][0]
kArr[1][1] = kArr[0][1] * kArr[0][1]

printks "kArr[0][0] = %f\nkArr[0][1] = %f\n", .1, kArr[0][0], kArr[0][1]
printks "kArr[1][0] = %f\nkArr[1][1] = %f\n", .1, kArr[1][0], kArr[1][1]

turnoff

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
