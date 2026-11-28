<CsTest>
description = "initializing a struct from an Opcode[] member holding an ended UDO reports failure"

[expect]
exit = "nonzero"
stderr_regex = ["could not copy structured value"]
stderr_excludes = ["array copy failed", "stale:", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Triple(value:k):k
  xout value * 3
endop

struct Holder items:Opcode[]

opcode MakeHolder(value:k):Holder
  objects:Opcode[] = create(opcoderef("Triple"), 1)
  unused:k init objects[0], value
  holder:Holder init objects
  xout holder
endop

saved@global:Holder init

instr 1
  saved = MakeHolder(3)
endin

instr 2
  target:Holder init saved
  value:k run target.items[0], 3
  printks "stale: %g\n", 0, value
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0.1 0.05
e 0.3
</CsScore>
</CsoundSynthesizer>
