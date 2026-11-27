<CsTest>
description = "an alias survives the creator holder being deleted"

[expect]
exit = 0
output = ["alias result:"]
output_excludes = ["not initialised", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

shared@global:Opcode = create(opcoderef("lincos"))
alias@global:Opcode = shared

instr 1
  result:k run alias, 0.5, 0, 10
  printks "alias result: %g\n", 1, result
endin

instr 2
  delete(shared)
endin
</CsInstruments>
<CsScore>
i 1 0 0.3
i 2 0.1 0.01
e
</CsScore>
</CsoundSynthesizer>
