<CsTest>
description = "Rebinding an Opcode object to a constant of the wrong type for an argument its body writes is a performance error"

[expect]
exit = "nonzero"
stderr = ["mismatching arguments"]
output = ["the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

opcode WS(text:S):k
  text = "written by the body"
  kOut strlenk text
  xout kOut
endop

instr 1
  ; The arguments are checked when the object is rebound: a number where the
  ; body writes a string is refused, and nothing is written for it.
  objects:Opcode[] = create(opcoderef("WS"), 1)
  Sv = "a variable"
  kR init objects[0], Sv
  kR perf objects[0], 4
endin

instr 2
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
e
</CsScore>
</CsoundSynthesizer>
