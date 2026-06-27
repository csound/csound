<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime
</CsOptions>
<CsInstruments>

instr 1
a1 diskin2 "fox.wav"
   out a1*0.1
   schedule(1,0.1,1) 
endin

instr 2
a1[] diskin2 "fox.wav"
   out a1[0]*0.1
   schedule(1,0.1,1) 
endin


instr 3
 eventi("e",0,0)
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 1 0
</CsScore>
</CsoundSynthesizer>
