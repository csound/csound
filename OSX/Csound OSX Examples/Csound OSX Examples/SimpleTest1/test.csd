<CsoundSynthesizer>
<CsOptions>
-o dac -+rtaudio=null -dm0
</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

instr 1

isl chnget "slider" 
ksl chnget "slider" 
ksl port ksl, 0.01, isl 
a2 expsegr 0.001,0.01,p4,p3-0.01, 0.001, 0.1, 0.001
a1 oscili a2, p5*(1+ksl), 1
outs a1,a1
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1
{ 10 CNT
i1 [$CNT*0.2] 0.5 0.5 [440*(2^[$CNT/12])]
}


</CsScore>
</CsoundSynthesizer>
