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
koff init 0
kts = p4 // timescale
a1 diskin "drumsMlp.wav",1,0,1
a3 lpcfilter a1,koff,1,gifn,1024,ksmps,gifw
koff += ksmps*kts
if koff > ftlen(gifn) then
 koff = 0
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
