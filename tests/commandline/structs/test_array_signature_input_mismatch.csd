<CsTest>
description = "Reject an array of a different struct type at a UDO input"
[expect]
exit = 1
stderr = ["Unable to find opcode entry for 'TakeVoice'"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
struct Voice value:i
struct Other value:i, extra:i
opcode TakeVoice(items:Voice[]):void
endop
instr 1
  item:Other init 1, 2
  items:Other[] fillarray item
  TakeVoice(items)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
