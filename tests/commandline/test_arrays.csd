<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

kArr[] init 4

kcounter = 0

until (kcounter >= 4) do

kArr[kcounter] = kcounter ^ 2

kcounter = kcounter + 1
od

printks "kArr[0] = %f\nkArr[1] = %f\n", .1, kArr[0], kArr[1]
printks "kArr[2] = %f\nkArr[3] = %f\n", .1, kArr[2], kArr[3]

turnoff

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
