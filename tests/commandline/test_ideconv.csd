<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

ifn ftgen 1,0,0,-1,"sweep.wav",0,0,1
ifn ftgen 2,0,0,-1,"rev.wav",0,0,1

instr 1
 swp:i[] init ftlen(1)
 inp:i[] init ftlen(2)
 copyf2array(swp,1)
 copyf2array(inp,2)
 outp:i[] deconv inp, swp
 copya2ftab outp, 1
 sig:a diskin2 "fox.wav"
 rev:a ftconv sig, 1, 64
     out rev*0.5
endin

</CsInstruments>
<CsScore>
i1 0 5
</CsScore>
</CsoundSynthesizer>
