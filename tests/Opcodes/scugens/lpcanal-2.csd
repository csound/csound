<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gifn ftgen 0,0,0,1,"fox.wav",0,0,1
gifw ftgen 0,0,1024,20,2,1

instr 1
k1 init 0
kts = p4
kcfs[],krms,kerr,kcps lpcanal k1,1,gifn,1024,ksmps,gifw
if kcps > 180 then
  kcps = 180
endif
a1 buzz 0dbfs, kcps, sr/(kcps*2), -1
a3 allpole a1*krms*kerr,kcfs
k1 += ksmps*kts
if k1 > ftlen(gifn) then
 k1 = 0
endif 
a3 dcblock a3
out a3
endin

</CsInstruments>
<CsScore>
i1 0 10 1
i1 10 10 .75
i1 20 10 1.5
</CsScore>
</CsoundSynthesizer>
