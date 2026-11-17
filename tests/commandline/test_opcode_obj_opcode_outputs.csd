<CsTest>
description = "An Opcode object given fewer outputs than its opcode has is refused at the first one missing, when Opcode outputs would take the object and its inputs"

[expect]
exit = 4
stderr = ["mismatching arguments"]
output = ["missing Four outarg 2", "missing FourK outarg 2", "missing FourArray outarg 2", "missing Mixed outarg 1", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Four():(Opcode,Opcode,Opcode,Opcode)
  object:Opcode = create(opcoderef("oscil"))
  xout object, object, object, object
endop

opcode FourK():(Opcode,Opcode,Opcode,k)
  object:Opcode = create(opcoderef("oscil"))
  xout object, object, object, 1
endop

opcode FourArray():(Opcode,Opcode,Opcode,k[])
  object:Opcode = create(opcoderef("oscil"))
  values:k[] init 2
  xout object, object, object, values
endop

opcode Mixed():(Opcode,k,k)
  object:Opcode = create(opcoderef("oscil"))
  xout object, 1, 2
endop

; given one output, outputs 2 and 3 would take the object and the
; Opcode input, and output 4 the empty slot past them
instr 1
  object:Opcode create opcoderef("Four")
  other:Opcode create opcoderef("oscil")
  first:Opcode run object, other
  turnoff
endin

instr 2
  object:Opcode create opcoderef("FourK")
  other:Opcode create opcoderef("oscil")
  first:Opcode run object, other
  turnoff
endin

instr 3
  object:Opcode create opcoderef("FourArray")
  other:Opcode create opcoderef("oscil")
  first:Opcode run object, other
  turnoff
endin

; given no output, output 1 would take the object, 2 the input
instr 4
  object:Opcode create opcoderef("Mixed")
  value:k init 1
  run object, value
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
