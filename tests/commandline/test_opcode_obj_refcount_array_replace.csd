<CsTest>
description = "an Opcode array can be replaced and shrunk before use"

[expect]
exit = 0
output = ["array replace result:"]
output_excludes = ["not initialised", "resized since init", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

instr 1
  objects:Opcode[] = create(opcoderef("lincos"), 2)
  objects = create(opcoderef("lincos"), 1)
  result:k run objects[0], 0.5, 0, 10
  printks "array replace result: %g\n", 1, result
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
e
</CsScore>
</CsoundSynthesizer>
