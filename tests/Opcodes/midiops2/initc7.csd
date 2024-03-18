<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -M0  ;;;realtime audio I/O with MIDI in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; expects MIDI controller input on channel 1
; run and move your midi controller to see result

imax = 1
imin = 0
ichan = 1 
ictlno = 7
 
	initc7	1, 7, 1			; start at max. volume
kamp	ctrl7	ichan, ictlno, imin, imax	; controller 7
	printk2	kamp
asig	oscil	kamp, 220, 1
	outs	asig, asig

endin

</CsInstruments>
<CsScore>
f 1 0 4096 10 1

i1 0 20

e
</CsScore>
</CsoundSynthesizer>
