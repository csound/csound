<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr	=	48000
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	
  asig = random:a(-1.0, 1.0) 
  asig = zdf_1pole(asig, line(220, p3, 10000), p4)
  outc(asig, asig)
endin

instr 2	
  asig = vco2(0.5, 220)
  asig = zdf_1pole(asig, line(220, p3, 10000), p4)
  outc(asig, asig)
endin

</CsInstruments>
<CsScore>
i1 0 4 0
i1 5 4 1
i1 10 4 2
i2 15 4 0
i2 20 4 1
i2 25 4 2 
</CsScore>
</CsoundSynthesizer>

