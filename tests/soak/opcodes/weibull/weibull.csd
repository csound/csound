<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1  	; every run time same values

ktri	weibull 100, 1
	printk .2, ktri			; look 
aout	oscili 0.8, 440+ktri, 1		; & listen
	outs	aout, aout
endin

</CsInstruments>
<CsScore>
; sine wave
f 1 0 16384 10 1

i 1 0 2
e
</CsScore>
</CsoundSynthesizer>

