<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

0dbfs = 1

// three different inputs
instr 1
 alp = diskin:a("fox.wav",1,0,1)/3
 ahp rand p4/3
 abp vco2 p4,100 
 af expon 100,p3,10000
 a3 spf alp,ahp,abp,af,0.707
 out a3
 a1 = 0
endin

// band-reject
instr 2
 a0 init 0
 anoi rand p4
 af expon 100,p3,10000
 as spf anoi,anoi,a0,af,0.3
 out as
endin

// self-oscillation
instr 3
 a0 init 0
 anoi rand p4
 af expon 100,p3,10000
 as spf anoi,a0,a0,af,0
 out as
endin

</CsInstruments>
<CsScore>
i1 0 10 0.2
i2 10 10 0.5
i3 20 10 0.004
</CsScore>
</CsoundSynthesizer>