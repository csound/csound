<CsTest>
description = "array element temporary holders do not destroy opcode instances"

[expect]
exit = 0
output = ["array temporary result:"]
output_excludes = ["not initialised", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

objects@global:Opcode[] = [create(opcoderef("lincos")), create(opcoderef("lincos"))]

instr 1
  result:k run objects[1], 0.5, 0, 10
  printks "array temporary result: %g\n", 1, result
endin

instr 2
  delete(objects)
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
i 2 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
