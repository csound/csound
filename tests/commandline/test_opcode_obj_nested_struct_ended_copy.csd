<CsTest>
description = "copying a struct with an ended Opcode member reports failure instead of running stale dataspace"

[expect]
exit = "nonzero"
stderr_regex = ["could not copy structured value"]
stderr_excludes = ["stale:", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Twice(value:k):k
  xout value * 2
endop

struct Inner val:i, opc:Opcode
struct Outer inner:Inner

opcode MakeOuter(x:i):Outer
  object:Opcode = create(opcoderef("Twice"))
  unused:k init object, 1
  inner:Inner init 1, object
  result:Outer init inner
  xout result
endop

saved@global:Outer init

instr 1
  saved = MakeOuter(1)
endin

instr 2
  target:Outer = saved
  stale:k run target.inner.opc, 3
  printks "stale: %g\n", 0, stale
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0.1 0.05
e
</CsScore>
</CsoundSynthesizer>
