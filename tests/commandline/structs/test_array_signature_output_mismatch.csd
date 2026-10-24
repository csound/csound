<CsTest>
description = "Reject an array of a different struct type at a UDO output"
[expect]
exit = 1
stderr = ["Unable to find opcode entry for 'MakeVoice'"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
struct Voice value:i
struct Other value:i, extra:i
opcode MakeVoice():Voice[]
  item:Voice init 7
  items:Voice[] fillarray item
  xout items
endop
instr 1
  items:Other[] MakeVoice
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
