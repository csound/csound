<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

myvar:i@global init 0
myarr:i@global[] fillarray 1,2,3

instr 1
print myvar
print myarr[0]
myarr[1] = 0
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
