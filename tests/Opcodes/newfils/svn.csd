<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

0dbfs = 1

instr 1
 as vco2 0dbfs,100
 kcf expseg 20,p3/2,20000,p3/2,20
 ahp,alp,abp,abr svn as,kcf,p5,p6,p7,p8
    out alp*p4
endin

</CsInstruments>
<CsScore>
// x - .2x^3 + .3x^5 - .1x^7
f1 0 16385 3 -1 1 0 1 0 -.2 0 .3 0 -.1
f2 0 8193 4 1 1
i1 0 10 0.1 10 0 0 0 ; linear
i1 + 10 0.1 10 0.25 0 0 ; 25% drive
i1 + 10 0.1 10 0.75 0 0 ; 75% drive
i1 + 10 0.1 10 1 0 0 ; 100% drive
i1 + 10 0.1 10 0.25 1 2 ; table 1  25% drive 
i1 + 10 0.1 10 0.75 1 2 ; table 1   75%
i1 + 10 0.1 10 1 1 2 ; table 1   100%
</CsScore>
</CsoundSynthesizer>