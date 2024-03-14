<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr	= 44100	
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	
  asig = random:a(-1.0, 1.0) 
  asig = zdf_2pole(asig, line(20, p3, 10000), 4, p4)

  outc(asig, asig)
endin

instr 2	
  asig = vco2(0.5, 220) 
  asig = zdf_2pole(asig, line(20, p3, 10000), 4, p4)
  outc(asig, asig)
endin

</CsInstruments>
<CsScore>
i1 0 4 0
i1 5 4 1 
i1 10 4 2
i1 15 4 3
i1 20 4 4
i1 25 4 5
i1 30 4 6

i2 40 4 0
i2 45 4 1 
i2 50 4 2
i2 55 4 3
i2 60 4 4
i2 65 4 5
i2 70 4 6

</CsScore>
</CsoundSynthesizer>

