<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent
-odac           -iadc     -n    ;;;realtime output
</CsOptions>
<CsInstruments>

sr	=	44100
ksmps	=	1000
nchnls	=	2

		instr	1	; logcurve test

kmod	phasor	1/p3
kout	expcurve kmod, p4

printks "mod = %f  out= %f\\n", 0.5, kmod, kout

		endin

/*--- ---*/
</CsInstruments>
<CsScore>

i1	0	5  2
i1	5	5  5
i1	10	5  30
i1	15	5  0.5

e
</CsScore>
</CsoundSynthesizer>
