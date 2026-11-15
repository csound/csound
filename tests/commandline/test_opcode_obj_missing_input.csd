<CsTest>
description = "An Opcode object given fewer inputs than its opcode needs is refused, at init or at its first performance call, and so is an Opcode[] run given fewer outputs"

[expect]
exit = 4
stderr = ["mismatching arguments"]
output = ["missing lincos inarg 3", "missing oscil.kkk inarg 2", "missing lincos outarg 1", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

instr 1 ; lincos (kkkOP) needs three
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  result:k init object, x, y0
  turnoff
endin

instr 2 ; oscil.kkk (kkjo) needs two
  object:Opcode create opcoderef("oscil"), 1
  amp:k init 1
  result:k init object, amp
  turnoff
endin

instr 3 ; an object only performed binds its inputs on its first call
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  result:k perf object, x, y0
  turnoff
endin

instr 4 ; Opcode[] run with no output, where lincos has one
  objects:Opcode[] create opcoderef("lincos"), 2
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  run objects, x, y0, y1
  turnoff
endin

instr 5
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
i 3 0.2 0.1
i 4 0.3 0.1
i 5 0.4 0.1
e
</CsScore>
</CsoundSynthesizer>
