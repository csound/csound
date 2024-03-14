<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1

a1 diskin "fox.wav",1,0,1
k1 randh  80, 2.5
a2 vco2  8, 220+k1
fsig pvsanal a1,1024,256,1024,1
fsig2 pvsanal a2,1024,256,1024,1
keps[] pvsceps fsig,30
kenv[] cepsinv keps
fenv tab2pvs r2c(kenv)
fvoc pvsfilter fsig2, fenv, 1
asig pvsynth fvoc

    out asig
endin


</CsInstruments>
<CsScore>
i1 0 60
</CsScore>
</CsoundSynthesizer>