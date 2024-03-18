<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>
sr=44100
ksmps=16
nchnls=2
0dbfs=1


gi_sine ftgen 0, 0, 65537, 10, 1

gkcut init 6000


instr modulation 
  gkcut = lfo(4000, 0.1) + 6000 
endin

instr bass	 

  iamp = ampdbfs(-12) 
  ipch = cps2pch(p4, 12)

  asig = vco2(0.5, ipch, 0)

  acut = expon:a(i(gkcut), p3, 200) 
  aout = diode_ladder(asig, acut, 8, 1, 4)

  aout *= expseg:a(1.0, p3 - 0.05, 1.0, 0.05, 0.001) 

  aout = limit(aout, -1.0, 1.0)

  outc(aout, aout)

endin


gipat[] init 8
gipat[0] = 6.00
gipat[1] = 7.00
gipat[2] = 6.00
gipat[3] = 7.00
gipat[4] = 5.07
gipat[5] = 6.07
gipat[6] = 5.08
gipat[7] = 6.08


instr player
  indx = p4

  ;; play instrument
  if(gipat[indx] > 0) then
    schedule("bass", 0, 0.2, gipat[indx])
  endif

  ;; temporal recursion
  schedule("player", 0.2, 0.1, (indx + 1) % lenarray(gipat))

endin

schedule("modulation", 0, -1)
schedule("player", 0, 0.1, 0)
event_i("e", 0, 0.1 * 128)

</CsInstruments>

<CsScore>  
</CsScore>

</CsoundSynthesizer>
