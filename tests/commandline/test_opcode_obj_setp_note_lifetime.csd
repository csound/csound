<CsTest>
description = "a setp override is owned by the object after its creating note ends"

[expect]
exit = 0
output = ["first: 10", "later: 10"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

object@global:Opcode = create(opcoderef("lincos"))

instr 1
  input:k init 1
  start:k init 0
  end:k init 10
  override:k init 5
  setp object, 0, override
  first:k run object, input, start, end
  printks "first: %g\n", 0, first
  turnoff
endin

instr 2
  input:k init 1
  start:k init 0
  end:k init 10
  later:k run object, input, start, end
  printks "later: %g\n", 0, later
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
s
i 2 0.3 0.1
e
</CsScore>
</CsoundSynthesizer>
