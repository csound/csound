<CsTest>
description = "An Opcode object of an opcode taking alternating k and a inputs, outch or mac, takes them from the first input, whatever its number of outputs"

[expect]
exit = 1
stderr = ["mismatching arguments"]
output = ["outch ran", "mac ran", "outch inarg 1, expected type: k, got: a", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 2
0dbfs = 1

; outch takes channel, signal, channel, signal..., and has no output
instr 1
  object:Opcode create opcoderef("outch")
  signal:a = 0
  run object, 1, signal, 2, signal
  printks "outch ran\n", 1
  turnoff
endin

; mac has one output
instr 2
  object:Opcode create opcoderef("mac")
  signal:a = 0
  sum:a run object, 1, signal
  printks "mac ran\n", 1
  turnoff
endin

instr 3
  object:Opcode create opcoderef("outch")
  signal:a = 0
  run object, signal, 1
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
