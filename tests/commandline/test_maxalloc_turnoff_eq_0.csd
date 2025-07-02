<CsoundSynthesizer>

<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	
  aout vco2 10000, 440
  out aout
endin

maxalloc 1, 1, 0

</CsInstruments>

<CsScore>
i1 0 2
i1 0 2
e
</CsScore>

</CsoundSynthesizer>
