<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o polynomial.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; The polynomial y=x^n where n is odd always produces a curve
; that traverses the range [-1, 1] when the input is within
; the same range.  Therefore, we can use one of these curves
; to make a nonlinear phasor that repeatedly reads a table 
; from beginning to end like a linear phasor (maintaining 
; continuity) but that distorts the waveform in the table.

instr 4	; This instrument demonstrates phase distortion with x^3

idur   = p3
iamp   = p4
ifreq  = p5
itable = p6
	
aenv linseg 0, .001, 1.0, idur - .051, 1.0, .05, 0	; declicking envelope
aosc phasor ifreq					; create a linear phasor
apd  polynomial aosc, 0, 0, 0, 1			; distort the phasor with x^3
aout tablei apd, itable, 1				; read a sine table with the nonlinear phasor
     outs aenv*aout*iamp, aenv*aout*iamp
		
endin

instr 5	; This instrument demonstrates phase distortion with x^11

idur   = p3
iamp   = p4
ifreq  = p5
itable = p6

aenv linseg 0, .001, 1.0, idur - .051, 1.0, .05, 0	; declicking envelope
aosc phasor ifreq					; create a linear phasor
apd  polynomial aosc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ; distort the phasor with x^11
aout tablei apd, itable, 1				; read a sine table with the nonlinear phasor
     outs aenv*aout*iamp, aenv*aout*iamp
		
endin

instr 6 ; This instrument crossfades between a pure sine and one distorted with x^11

idur   = p3
iamp   = p4
ifreq  = p5
itable = p6
	
aenv	linseg	0, .001, 1.0, idur - .051, 1.0, .05, 0	; declicking envelope
aosc	phasor	ifreq					; create a linear phasor
aout3	tablei	aosc, itable, 1				; read a sine table without the linear phasor
apd11	polynomial aosc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 ; distort the phasor with x^11
aout11	tablei	apd11, itable, 1			; read a sine table with the nonlinear phasor
kamount	linseg	1.0, 0.05, 0.9, 1.0, 0.0		; crossfade between two outputs
aout	= aout3*kamount + aout11*(1.0 - kamount)
        outs aenv*aout*iamp, aenv*aout*iamp	
	
endin
</CsInstruments>
<CsScore>
f1 0 16385 10 1	; sine wave

; descending "just blues" scale

t 0 100
i4 0 .333 .7 512     1
i. + .    .  448
i. + .    .  384
i. + .    .  360
i. + .    .  341.33
i. + .    .  298.67
i. + 2    .  256
s

t 0 100
i5 0 .333 .7 512     1
i. + .    .  448
i. + .    .  384
i. + .    .  360
i. + .    .  341.33
i. + .    .  298.67
i. + 2    .  256
s

t 0 100
i6 0 .333 .7 512     1
i. + .    .  448
i. + .    .  384
i. + .    .  360
i. + .    .  341.33
i. + .    .  298.67
i. + 2    .  256

e

</CsScore>
</CsoundSynthesizer>
