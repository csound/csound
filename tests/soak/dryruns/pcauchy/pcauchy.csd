<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1  	; every run time same values

kalpha	pcauchy	1000
	printk	.2, kalpha		; look
aout	oscili	0.8, 440+kalpha, 1	; & listen
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
