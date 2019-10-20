<CsoundSynthesizer>
<CsInstruments>

sr	=	48000
ksmps	=	100
nchnls	=	2

instr	1	; gainslider test

  ; uncomment for realtime midi
  ;kmod	ctrl7	1, 1, 0, 127

  ; uncomment for non realtime
  km0d phasor 1/10
  kmod scale km0d, 127, 0

  kout	gainslider	kmod

  aout	diskin2	"fox.wav", 1, 0, 1

  aout	=	aout*kout

  outs	aout, aout

endin

</CsInstruments>
<CsScore>
i1	0	30
e
</CsScore>
</CsoundSynthesizer>
