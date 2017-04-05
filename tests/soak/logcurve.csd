<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent
-odac           -iadc     -d    ;;;realtime output
</CsOptions>
<CsInstruments>

sr	=	48000
ksmps	=	100
nchnls	=	2

		instr	1	; logcurve test

kmod	phasor	1/p3

kout	logcurve kmod, p4

	printks "kmod = %f  kout = %f\\n", 0.1, kmod, kout

		endin

</CsInstruments>
<CsScore>

i1	0	10 2
i1	10	10 30
i1	20	10 0.5

e
</CsScore>
</CsoundSynthesizer>
