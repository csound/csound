<CsoundSynthesizer>
<CsInstruments>

sr	=	48000
ksmps	=	1000
nchnls	=	2

instr	1	; logcurve test

  kmod	phasor	1/p3
  kout	expcurve kmod, p4
  asig = poscil:a(1, kout)
  outs asig, asig
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
