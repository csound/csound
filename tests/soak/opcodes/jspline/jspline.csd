<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kamp    init p4
kcpsmin init 2
kcpsmax init 20

ksp  jspline kamp, kcpsmin, kcpsmax
aout pluck 1, 200+ksp, 1000, 0, 1
aout dcblock aout	;remove DC
     outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 10 2	;a bit jitter
i 1 8 10 10	;some more
i 1 16 10 20	;lots more
e
</CsScore>
</CsoundSynthesizer>
