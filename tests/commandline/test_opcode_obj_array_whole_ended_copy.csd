<CsTest>
description = "whole-array assignment of an Opcode[] containing an ended UDO reports failure instead of keeping stale data"

[expect]
exit = "nonzero"
stderr = ["could not copy opcode object array"]
output_excludes = ["stale: 9", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Twice(value:k):k
  xout value * 2
endop

opcode Triple(value:k):k
  xout value * 3
endop

objects@global:Opcode[] = [create(opcoderef("Triple"))]
saved@global:Opcode = create(opcoderef("Twice"))

instr 1
  initial:k run saved, 1
  objects = [saved]
  turnoff
endin

instr 2
  target:Opcode[] = [create(opcoderef("Triple"))]
  target = objects
  stale:k run target[0], 3
  printks "stale: %g\n", 0, stale
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.2 0.1
e
</CsScore>
</CsoundSynthesizer>
