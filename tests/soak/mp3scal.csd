<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
nchnls=2
ksmps=64
sr=44100
instr 1
SFile = p4
p3 = mp3len(SFile)/p5
a1,a2,k2 mp3scal SFile,p5,1,1
 outs a1,a2

endin

</CsInstruments>
<CsScore>
i1.1 0 1 "beats.mp3" .75
</CsScore>
</CsoundSynthesizer>
