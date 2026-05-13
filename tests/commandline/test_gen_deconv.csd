<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls = 2

ifn ftgen 1,0,0,-1,"sweep.wav",0,0,1
ifn ftgen 2,0,0,-1,"rev.wav",0,0,1
ifn ftgen 3,0,0,"deconv",1,2

instr 1
 sig:a mpulse 1,1
 rev:a ftconv  sig,3, 64
 rev2:a reverb sig,1
 rev2 delay rev2,64/sr
 err:k rms rev2-rev*0.95477
 if err > 0.00001 then
  printks "error rms: %f\n", 1, err
  exitnowk(-1)
 endif
     out rev2,rev
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
