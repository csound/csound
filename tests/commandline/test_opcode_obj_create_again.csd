<CsTest>
description = "create on an Opcode object that holds another opcode makes it the new one, alone or in an array, keeps none of the old one's arguments, and a run that bound the old one is refused, where a run on a copy of it runs"

[expect]
exit = 3
stderr = ["could not set arg 0", "opcode object not initialised"]
output = ["from prints|", "from prints[] 1|", "from prints[] 2|", "the same again|", "a copy ran on 5", "the next note ran"]
output_excludes = ["setp taken", "lincos ran on", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; printks takes a format, a period and values; prints a format and values
instr 1
  object:Opcode = create(opcoderef("printks"))
  object = create(opcoderef("prints"))
  init(object, "from prints|\n")
  turnoff
endin

instr 2
  objects:Opcode[] = create(opcoderef("printks"), 2)
  objects = create(opcoderef("prints"), 2)
  count:i[] fillarray 1, 2
  run objects, "from prints[] %d|\n", count
  turnoff
endin

instr 3
  object:Opcode = create(opcoderef("prints"))
  object = create(opcoderef("prints"))
  init(object, "the same again|\n")
  turnoff
endin

; the new object has bound nothing yet: setp has no input to set
instr 4
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  result:k init object, x, y0, y1
  object = create(opcoderef("oscil"), 1)
  setp object, 0, x
  printks "setp taken\n", 0
  turnoff
endin

; run bound lincos at init, and create then gave the object phasor,
; which is smaller: run has no arguments to perform it with
instr 5
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  result:k run object, x, y0, y1
  object = create(opcoderef("phasor"))
  printks "lincos ran on %d\n", 1, result
endin

instr 6
  objects:Opcode[] = create(opcoderef("lincos"), 2)
  x:k[] fillarray 0.5, 0.25
  y0:k init 0
  y1:k init 10
  results:k[] init 2
  results run objects, x, y0, y1
  objects = create(opcoderef("phasor"), 2)
  printks "lincos ran on %d\n", 1, results[0]
endin

; a copy made at every cycle keeps the dataspace run bound
instr 7
  first:Opcode create opcoderef("lincos")
  copy:Opcode = first
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  result:k run copy, x, y0, y1
  printks "a copy ran on %d\n", 1, result
endin

instr 8
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
i 3 0.2 0.1
i 4 0.3 0.1
i 5 0.4 0.1
i 6 0.5 0.1
i 7 0.6 0.1
i 8 0.7 0.1
e
</CsScore>
</CsoundSynthesizer>
