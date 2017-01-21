<CsoundSynthesizer>
<CsOptions>
-o dac
;-+rtaudio=null
;-dm0
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
a1 oscili a2, ksl, 1
outs a1,a1
endin

</CsInstruments>
<CsScore>
f 1 0 16384 10 1

i 1 0 36000 0.5
</CsScore>
</CsoundSynthesizer>
