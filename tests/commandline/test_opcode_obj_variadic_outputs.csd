<CsTest>
description = "An Opcode object of an opcode whose variable outputs follow a fixed one, delayr (aX), takes one output or two"

[expect]
exit = 0
output = ["delayr with one output ran", "delayr with two outputs ran"]
output_excludes = ["mismatch", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; the inputs follow both of delayr's output slots, given or not
instr 1
  object:Opcode create opcoderef("delayr")
  signal:a run object, 0.1
  prints "delayr with one output ran\n"
  turnoff
endin

instr 2
  object:Opcode create opcoderef("delayr")
  signal:a, index:a run object, 0.1
  prints "delayr with two outputs ran\n"
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
e
</CsScore>
</CsoundSynthesizer>
