; scales.orc
;
; This orchestra works with the output of the ScaleGen program.
;
; Anthony Kozar
; July 6, 2004

sr      = 44100
kr      = 4410
ksmps	= 10
nchnls  = 1


giSine		ftgen 0, 0, 8193, 10, 1, 0.5, 0.333, 0.25, 0.2	; saw approximation


; Parameters:
;
;	p3	duration
;	p4	amplitude
;	p5	frequency

instr 1

	idur	= p3
	iamp	= p4

	; interpret the pitch value as octave.pitchclass but for 19-tone ET
	ioct		=	int(p5)						; the octave 
	ipchclass	=	100*frac(p5)				; the pitch class (0-18)
	ifreq		=	cpsoct(ioct+ipchclass/19)	; convert it to oct format
		
	aosc	oscili	iamp, ifreq, giSine
	aout	linen	aosc, 0.01, idur, 0.05
	
			out		aout
endin
