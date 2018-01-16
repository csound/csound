<CsoundSynthesizer>

<CsInstruments>
nchnls = 2
sr = 44100
0dbfs = 1
ksmps = 64

giSine ftgen 0, 0, 4096, -10, 1

instr 1
aenv xadsr 0.1, 0.1, 0.5, 1, 0
aosc oscili aenv, 440, giSine, 0
outs1 aosc

endin

instr 2
aenv expseg 0.001, 0.1, 1, 0.1, 0.5, p3-1.21, 0.5, 1, 0.001
aosc oscili aenv, 440, giSine, 0
outs2 aosc

endin
</CsInstruments>

<CsScore>
f0 10

i 1.00001 0.000000 1.500000
i 1.00002 3.500000 4.000000

;;i 2.00001 0.000000 1.500000
;;i 2.00002 3.500000 4.000000

</CsScore>

</CsoundSynthesizer>
