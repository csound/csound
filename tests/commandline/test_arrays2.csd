<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

gkArr[] init 4

instr 1

prints "instr 1 - settings gkArr values and printing\n"
kcounter = 0

until (kcounter >= 4) do

gkArr[kcounter] = kcounter ^ 2

kcounter = kcounter + 1
od

printks "gkArr[0] = %f\ngkArr[1] = %f\n", .1, gkArr[0], gkArr[1]
printks "gkArr[2] = %f\ngkArr[3] = %f\n", .1, gkArr[2], gkArr[3]

turnoff

endin


instr 2

prints "instr 2 - reading gkArr values and printing\n"
printks "gkArr[0] = %f\ngkArr[1] = %f\n", .1, gkArr[0], gkArr[1]
printks "gkArr[2] = %f\ngkArr[3] = %f\n", .1, gkArr[2], gkArr[3]

turnoff

endin

</CsInstruments>

<CsScore>
i1 0 .5
i2 .5 .5
</CsScore>

</CsoundSynthesizer>
