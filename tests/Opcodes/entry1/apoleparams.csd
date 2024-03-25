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
kper init sr
a1 diskin "flute.aiff",1,0,1
kcfs[],krms,kerr,kcps lpcanal a1,1,512,1024,10,gifw
kpar[] apoleparams kcfs
if kper >= sr then
kcnt = 0
kper = 0
endif
kper += ksmps
while kcnt < 5 do
  printf "t:%.1f filter %d freq: %.1f bw: %.1f\n", kcnt+1, times:k(), kcnt, kpar[kcnt], kpar[kcnt+1]
  kcnt += 1
od
out a1
endin

</CsInstruments>
<CsScore>
i1 0 20
</CsScore>
</CsoundSynthesizer>

