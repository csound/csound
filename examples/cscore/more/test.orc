sr      = 44100
kr      = 4410
ksmps	= 10
nchnls  = 1


giSine		ftgen 0, 0, 8192, 10, 1						; sine table
giCosEnv	ftgen 0, 0, 8192, -19, 1, .5, 270, .5		; inverted cosine envelope



; Parameters:
;
;	p3	duration
;	p4	amplitude
;	p5	frequency

instr 1

	idur	= p3
	iamp	= p4
	ifreq	= p5
	
	ifnum	= giSine									; table number constants
	ienvnum	= giCosEnv
	ilen	= ftlen(ifnum)
	ienvlen	= ftlen(ienvnum)
	
	aenvph	phasor	1/idur								; envelope oscillator
	aenv	tablei	aenvph*ienvlen, ienvnum
	aphase	phasor	ifreq								; waveform oscillator
	asig	tablei	aphase*ilen, ifnum
	
			out		iamp*aenv*asig
			
endin
