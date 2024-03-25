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
  alp, ahp zdf_1pole_mode asig, line(220, p3, 10000)
  asig = (p4 == 0) ? alp : ahp
  outc(asig, asig)
endin

instr 2	
  asig = vco2(0.5, 220)
  alp, ahp zdf_1pole_mode asig, line(220, p3, 10000)
  asig = (p4 == 0) ? alp : ahp
  outc(asig, asig)
endin

</CsInstruments>
<CsScore>
i1 0 4 0
i1 5 4 1
i2 10 4 0
i2 15 4 1
</CsScore>
</CsoundSynthesizer>

