<CsTest>
description = "setp and getp on an Opcode object refuse a NaN index, or one too large to convert, rather than take it as an argument"

[expect]
exit = 4
stderr = ["argument index out of range"]
output = ["setp at 1: 5.125", "getp at 1: 12", "getp at -1: 11", "the next note ran"]
output_excludes = ["a large setp index", "a large getp index", "a NaN setp index", "a NaN getp index", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Two(value:k):(k,k)
  xout value + 1, value + 2
endop

; lincos (kkkOP) given 0.5, 0 and 10 gives 5, and 5.125 with its second
; input set to 0.25. A refused setp ends the note before it prints.
instr 1
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 1, 0.25
  result:k run object, x, y0, y1
  printks "setp at 1: %g\n", 0, result
  turnoff
endin

instr 2 ; 2^32 + 1
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 4294967297, 0.25
  result:k run object, x, y0, y1
  printks "a large setp index was set: %g\n", 0, result
  turnoff
endin

; Two's outputs are 11 and 12.
instr 3
  object:Opcode create opcoderef("Two")
  value:k init 10
  first:k, second:k run object, value
  read:k getp object, 1
  printks "getp at 1: %g\n", 0, read
  turnoff
endin

instr 4 ; a negative index reads the first output, as getp's manual says
  object:Opcode create opcoderef("Two")
  value:k init 10
  first:k, second:k run object, value
  read:k getp object, -1
  printks "getp at -1: %g\n", 0, read
  turnoff
endin

instr 5 ; 2^32
  object:Opcode create opcoderef("Two")
  value:k init 10
  first:k, second:k run object, value
  read:k getp object, 4294967296
  printks "a large getp index was read: %g\n", 0, read
  turnoff
endin

instr 6 ; a NaN
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  nan:k = sqrt(-1)
  setp object, nan, 0.25
  result:k run object, x, y0, y1
  printks "a NaN setp index was set: %g\n", 0, result
  turnoff
endin

instr 7 ; a NaN
  object:Opcode create opcoderef("Two")
  value:k init 10
  first:k, second:k run object, value
  nan:i = sqrt(-1)
  read:k getp object, nan
  printks "a NaN getp index was read: %g\n", 0, read
  turnoff
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
