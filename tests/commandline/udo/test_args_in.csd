<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
opcode Test(sig:k,fn:i):(a)
 a1 flooper2 sig,1,0, 3, 0.1,fn
xout a1
endop

gk1 init 0

i1 ftgen 1, 0, 0, 1, "../fox.wav", 0, 0, 1
instr 1
a1 Test 0.5,1
out a1
gk1 += downsamp(a1)
endin

instr 2
i1 = i(gk1)
if qnan(i1) > 0 || i1 == 0 then
 exitnow(-1)
endif
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0.5 1
</CsScore>
</CsoundSynthesizer>