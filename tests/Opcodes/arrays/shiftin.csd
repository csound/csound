<CsoundSynthesizer>
<CsOptions>
-d -odac
</CsOptions>
<CsInstruments>

instr 1
ideltime = 0.5
kDel[] init sr*0.5
a1 diskin2 "fox.wav",1,0,1
a2 shiftout kDel
kDel shiftin a1
     out a1 + a2
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
