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
a2 diskin "drumsMlp.wav",1,0,1
a3 lpcfilter a2,a1,1,128,1024,64,gifw
a3 dcblock2 a3
out a3
endin

</CsInstruments>
<CsScore>
i1 0 30
</CsScore>
</CsoundSynthesizer>
