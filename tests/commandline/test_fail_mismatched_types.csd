<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

myvar@global:i init 2
myarr@global:i[] fillarray 1,2,3

instr 1
 myarr:k[] init 4
 myvar:k = 4
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
