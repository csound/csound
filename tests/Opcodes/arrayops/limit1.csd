<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>


instr 1

iArg1[] fillarray 1,2,3
iRes[] limit1 iArg1/2.5
ik init 0

while ik < lenarray(iRes) do
 print iRes[ik]
 ik += 1
od

endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>