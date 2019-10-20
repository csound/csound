<CsoundSynthesizer>
<CsInstruments>

sr	= 48000
ksmps	= 100
nchnls	= 2

instr	1	; logcurve test
  kmod	phasor	1/p3
  kout	logcurve kmod, p4
  asig = poscil:a(kmod, kout)
  outs asig, asig
endin

</CsInstruments>
<CsScore>

i1	0	10 2
i1	11	10 30
i1	20	10 0.5

e
</CsScore>
</CsoundSynthesizer>
