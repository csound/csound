<CsTest>
description = "initializing a nested struct from an Opcode[] member holding an ended UDO reports failure"

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

struct Inner items:Opcode[]
struct Outer inner:Inner

opcode MakeOuter(value:k):Outer
  objects:Opcode[] = create(opcoderef("Triple"), 1)
  unused:k init objects[0], value
  inner:Inner init objects
  result:Outer init inner
  xout result
endop

saved@global:Outer init

instr 1
  saved = MakeOuter(3)
endin

instr 2
  target:Outer init saved
  value:k run target.inner.items[0], 3
  printks "stale: %g\n", 0, value
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0.1 0.05
e 0.3
</CsScore>
</CsoundSynthesizer>
