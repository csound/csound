<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>


instr 1

iArg1[] fillarray 10,8,22
iArg2[] fillarray 4,5,6
iRes[] fmod iArg1,iArg2
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
