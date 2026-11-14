<CsTest>
description = "setp on an Opcode object refuses an index past the inputs its call filled, given or defaulted, and sets every defaulted one"

[expect]
exit = 5
stderr = ["could not set arg 4", "could not set arg 1", "could not set arg 3", "could not set arg 2"]
output = ["lincos's second default set to 2: 1.4645", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
0dbfs = 1

opcode Twice(value:k):k
  xout value * 2
endop

opcode Total(values:k[]):k
  xout sumarray(values)
endop

opcode Named(definition:OpcodeDef, value:k):k
  xout value * 2
endop

; Each index is one past the inputs the call gave, and the call left out no
; optional input, so there is no argument there.

instr 1 ; oscil.kkk (kkjo) given all four
  object:Opcode create opcoderef("oscil"), 1
  amp:k init 1
  freq:k init 10
  setp object, 4, freq
  result:k run object, amp, freq, -1, 0
  turnoff
endin

instr 2 ; a UDO given its one input
  object:Opcode create opcoderef("Twice")
  value:k init 21
  setp object, 1, value
  result:k run object, value
  turnoff
endin

instr 3 ; max.k (kz) given three
  object:Opcode create opcoderef("max"), 2
  a:k init 1
  b:k init 5
  c:k init 3
  setp object, 3, b
  result:k run object, a, b, c
  turnoff
endin

instr 4 ; a UDO given its one array input
  object:Opcode create opcoderef("Total")
  values:k[] fillarray 1, 2, 3
  setp object, 1, values
  result:k run object, values
  turnoff
endin

instr 5 ; a UDO given an input of a named type, and a number
  object:Opcode create opcoderef("Named")
  definition:OpcodeDef init "oscil"
  value:k init 21
  setp object, 2, value
  result:k run object, definition, value
  turnoff
endin

; lincos (kkkOP) given three defaults its last two inputs: both are its own.
instr 6
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 4, 2
  result:k run object, x, y0, y1
  printks "lincos's second default set to 2: %.4f\n", 0, result
  turnoff
endin

instr 7
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
e
</CsScore>
</CsoundSynthesizer>
