<CsTest>
description = "<CsSampleB> takes its number unquoted, as <CsFileB> takes its name"
args = ["-nd"]

[expect]
exit = 0
output = ["sample 1"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "sample %d\n", filevalid("soundin.9733")
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
<CsSampleB filename=9733>
aGVsbG8K
</CsSampleB>
</CsoundSynthesizer>
