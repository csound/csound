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
  asig = zdf_ladder(asig, expon(220, p3, 5000), p4)
  outc(asig, asig)
endin

instr 2	
  asig = vco2(0.5, 220)
  asig = zdf_ladder(asig, expon(220, p3, 5000), p4)
  outc(asig, asig)
endin

instr 3	
  asig = vco2(0.5, p4)
  asig = zdf_ladder(asig, expon(5000, p3, 200), 0.5 + p5* 24)
  asig *= madsr(0.05, 0, 1, 0.25)
  outc(asig, asig)
endin


instr play_instr3
  schedule(3, 0, 0.25, mtof(48 + (p4 % 2) * 12), p4 / 16)

  if(p4 < 16) then 
    schedule("play_instr3", 0.25, 0.25, p4 + 1)
  else
    event_i("e", 0.5,0)
  endif 
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 2 0.5
i1 + . 1 
i1 + . 4 
i1 + . 10 
i1 + . 18 
i1 + . 24.5 
i2 12 2 0.5
i2 + . 1 
i2 + . 4 
i2 + . 10 
i2 + . 18 
i2 + . 24.5 

s
i "play_instr3" 0 0.25 0 
f0 60
</CsScore>
</CsoundSynthesizer>

