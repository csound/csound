<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>


instr 1

iArg[] fillarray 1,2,3
iRes[] cbrt iArg
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