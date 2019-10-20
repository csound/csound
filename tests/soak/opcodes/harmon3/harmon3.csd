<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilow = p4					;lowest value to harmonize			
aout diskin2 "fox.wav", 1, 0, 1
koct, kamp pitch aout, .01, 6, 10, 10		;track pitch
asig harmon3 aout, koct, .9, 1.5, 0.7, 0, ilow
     outs (asig + aout)*.4, (asig + aout)*.4	;mix dry&wet signal

endin
</CsInstruments>
<CsScore>

i1 0 2.7 8.8
i1 3 2.7 8.2
i1 6 2.7 7.0

e
</CsScore>
</CsoundSynthesizer>

