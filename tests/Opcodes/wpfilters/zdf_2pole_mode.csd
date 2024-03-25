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
  asigs[] init 3
  asigs[0], asigs[1], asigs[2] zdf_2pole_mode asig, line(20, p3, 10000), 4 

  asig = asigs[p4]
  outc(asig, asig)
endin

instr 2	
  asig = vco2(0.5, 220) 
  asigs[] init 3
  asigs[0], asigs[1], asigs[2] zdf_2pole_mode asig, line(20, p3, 10000), 4 
  outc(asigs[p4], asigs[p4])
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

