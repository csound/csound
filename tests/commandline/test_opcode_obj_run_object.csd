<CsTest>
description = "An Opcode object of run, whose outputs are any number, places its inputs after them, with an output or none"

[expect]
exit = 0
output = ["oscili through run: 1", "printks through run|"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

instr 1
  inner:Opcode = create(opcoderef("oscili"))
  outer:Opcode = create(opcoderef("run"))
  signal:a = run(outer, inner, 0.5, 100)
  printks "oscili through run: %d\n", 1, rms(signal) > 0
  turnoff
endin

; run with no output keeps one empty slot before its inputs
instr 2
  inner:Opcode = create(opcoderef("printks"))
  outer:Opcode = create(opcoderef("run"))
  run outer, inner, "printks through run|\n", 1
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
e
</CsScore>
</CsoundSynthesizer>
