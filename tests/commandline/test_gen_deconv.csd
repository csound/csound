<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls = 2

ifn ftgen 1,0,0,-1,"sweep.wav",0,0,1
ifn ftgen 2,0,0,-1,"rev.wav",0,0,1
ifn ftgen 3,0,0,"deconv",1,2,2

instr 1
 sig:a diskin2 "fox.wav"
 rev:a, rev2:a ftconv sig*0.49, 3, 64
     out rev, rev2
endin

</CsInstruments>
<CsScore>
i1 0 5
</CsScore>
</CsoundSynthesizer>
