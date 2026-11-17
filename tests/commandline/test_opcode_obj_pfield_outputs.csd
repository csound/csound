<CsTest>
description = "An Opcode object writes an output into a p-field, as a call does, where the output takes an i, and getp reads it"

[expect]
exit = 0
output = ["init.i into p4: 7", "abs.i into p5: 3", "getp from p5: 3"]
output_excludes = ["outarg", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; init's overload 7 is init.i (III...), abs's overload 4 is abs.i (i)
instr 1
  object:Opcode create opcoderef("init"), 7
  p4 init object, 7
  prints "init.i into p4: %d\n", p4
  absolute:Opcode create opcoderef("abs"), 4
  p5 init absolute, -3
  prints "abs.i into p5: %d\n", p5
  value:i getp absolute, 0
  prints "getp from p5: %d\n", value
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1 0 0
e
</CsScore>
</CsoundSynthesizer>
