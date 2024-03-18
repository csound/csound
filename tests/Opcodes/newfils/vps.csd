<CsoundSynthesizer>
<CsOptions>
-o dac -d
</CsOptions>
<CsInstruments>
0dbfs = 1
nchnls=1
ksmps = 1

instr 1
 kd = oscili:k(0.5,2) + 0.5
 kv = oscili:k(0.5,1.5) + 0.5
 aph = vps(phasor(p5),kd,kv)
 asig = p4*tablei:a(aph,-1,1,0.25,1)
 out(linenr(asig,0.1,0.1,0.01))
endin

</CsInstruments>
<CsScore>

i 1 0 10 0.5 110 


</CsScore>
</CsoundSynthesizer>




