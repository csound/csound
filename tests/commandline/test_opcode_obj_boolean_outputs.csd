<CsTest>
description = "An Opcode object of init for booleans, whose outputs are any number of b or B, takes them"

[expect]
exit = 1
stderr = ["mismatching arguments"]
output = ["init.b through an object: 1", "init.B through an object: 1", "init.b outarg 1, expected type: b, got: k", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; init's overload 0 is init.b (vvv...), 1 is init.B (VVV...)
instr 1
  object:Opcode create opcoderef("init"), 0
  flag:b init 0
  flag init object, 1
  prints "init.b through an object: %d\n", flag ? 1 : 0
  turnoff
endin

instr 2
  object:Opcode create opcoderef("init"), 1
  flag:B init 0
  flag init object, 1
  printks "init.B through an object: %d\n", 1, flag ? 1 : 0
  turnoff
endin

instr 3
  object:Opcode create opcoderef("init"), 0
  value:k init 0
  value init object, 1
  turnoff
endin

instr 4
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
i 3 0.2 0.1
i 4 0.3 0.1
e
</CsScore>
</CsoundSynthesizer>
