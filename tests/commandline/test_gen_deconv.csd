<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

ifn ftgen 1,0,0,-1,"sweep.wav",0,0,1
ifn ftgen 2,0,0,-1,"rev.wav",0,0,1
ifn ftgen 3,0,0,"deconv",2,1

instr 1
 sig:a diskin2 "fox.wav"
 rev:a ftconv sig, 3, 64
     out rev*0.5
endin

</CsInstruments>
<CsScore>
i1 0 5
</CsScore>
</CsoundSynthesizer>
