<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gifw ftgen 0,0,1024,20,2,1

instr 1
a1 diskin "fox.wav",1,0,1
kcfs[],krms,kerr,kcps lpcanal a1,1,128,1024,64,gifw
if kcps > 180 then
  kcps = 180
endif
a2 buzz 0dbfs, kcps, sr/(kcps*2), -1
a3 allpole a2*krms*kerr,kcfs
a3 dcblock a3
out a3
endin

</CsInstruments>
<CsScore>
i1 0 30
</CsScore>
</CsoundSynthesizer>
