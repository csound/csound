<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kres = p4
inumlayer = 4

kenv linseg 0, p3*.1, 1, p3*.8, 1, p3*.1, 0 	;envelope
asig vco .3 * kenv, 220, 1			;sawtooth
kcut line 30, p3, 1000				;note: kcut is not in Hz
alx  lowresx asig, kcut, kres, inumlayer	;note: kres is not in dB
aout balance alx, asig				;avoid very loud sounds
     outs aout, aout

endin
</CsInstruments>
<CsScore>
;sine wave
f 1 0 16384 10 1

i 1 0 5 1
i 1 + 5 3
i 1 + 5 20
e
</CsScore>
</CsoundSynthesizer>
