<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 4
nchnls = 2
0dbfs = 1

instr 1

asig init 0

kcounter = 0

until (kcounter >= 4) do

asig[kcounter] = kcounter ^ 2

kcounter = kcounter + 1
od

printks "asig[0] = %f\nasig[1] = %f\n", .1, asig[0], asig[1]
printks "asig[2] = %f\nasig[3] = %f\n", .1, asig[2], asig[3]

turnoff

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
