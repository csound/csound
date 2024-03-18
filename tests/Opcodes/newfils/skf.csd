<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1
 alp = diskin:a("fox.wav",1,0,1)/3
 af expon 100,p3,10000
 a3 skf alp,af,1.516,p5
 out a3
endin

</CsInstruments>
<CsScore>
i1 0 10 0.2 0  ; lowpass
i1 10 10 0.2 1 ; highpass
</CsScore>
</CsoundSynthesizer>
