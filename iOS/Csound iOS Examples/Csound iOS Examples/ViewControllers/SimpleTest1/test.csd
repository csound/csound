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

instr 2
asig,asig1 ins
a1 init 0
a2 init 0
ksl chnget "slider"
a1 delay asig+a2*0.5*ksl,0.3
a2 delay asig+a1*0.5*ksl,0.8
outs a2,a1
endin

instr 3

asig,asig1 ins

ksl chnget "slider"
fsig1 pvsanal asig,2048,256,2048,1
fsig2 pvscale fsig1,0.75+ksl,1
fsig3 pvscale fsig1,(0.75+ksl)*1.25,1
fsig4 pvsmix fsig2, fsig3
a1 pvsynth fsig4 
   outs a1, a1
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1
{ 10 CNT
i1 [$CNT*0.2] 0.5 0.5 [440*(2^[$CNT/12])]
}
/*i 2 0.1 10000 

i 3 0.1 10000*/

</CsScore>
</CsoundSynthesizer>
