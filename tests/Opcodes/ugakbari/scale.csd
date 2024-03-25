<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent
-odac           -iadc     -d    ;;;realtime output
</CsOptions>
<CsInstruments>

sr	=	44100
ksmps	=	10
nchnls	=	2

/*--- ---*/

	instr	1	; scale test

kmod	ctrl7	1, 1, 0, 1
	printk2	kmod

kout	scale	kmod, 0, -127
	printk2	kout

	endin

/*--- ---*/
</CsInstruments>
<CsScore>

i1	0	8888

e
</CsScore>
</CsoundSynthesizer>
