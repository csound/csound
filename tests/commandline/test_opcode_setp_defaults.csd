<CsTest>
description = "setp on an Opcode object sets a given input, a defaulted one and one of a list of variable length, and refuses an index past them, a type the slot cannot take, or a slot a failed call left empty"

[expect]
exit = 4
stderr = ["could not set arg 5", "could not set arg 0", "could not set arg 1", "mismatching arguments"]
output = [
  "lincos's first input set to 0.25: 1.4645",
  "lincos's first default set to 0.5: 0.0000",
  "kP's default set to 2: 42",
  "kp's default accepted",
  "max's third input set to 7: 7",
  "the next note ran",
]
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

opcode OldBigP, k, kP
  kv, kmul xin
  xout kv * kmul
endop

opcode OldP, k, kp
  kv, imul xin
  xout kv * imul
endop

instr 1 ; lincos (kkkOP) given three: a given input
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 0, 0.25
  result:k run object, x, y0, y1
  printks "lincos's first input set to 0.25: %.4f\n", 0, result
  turnoff
endin

instr 2 ; lincos given three: the first defaulted input
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 3, 0.5
  result:k run object, x, y0, y1
  printks "lincos's first default set to 0.5: %.4f\n", 0, result
  turnoff
endin

instr 3 ; a classic UDO's k-rate optional input, left out
  object:Opcode create opcoderef("OldBigP")
  value:k init 21
  setp object, 1, 2
  result:k run object, value
  printks "kP's default set to 2: %g\n", 0, result
  turnoff
endin

instr 4 ; a classic UDO's i-rate optional input, left out
  object:Opcode create opcoderef("OldP")
  value:k init 21
  setp object, 1, 2
  result:k run object, value
  printks "kp's default accepted\n", 0
  turnoff
endin

instr 5 ; max.k (kz) given three: the last of the list
  object:Opcode create opcoderef("max"), 2
  a:k init 1
  b:k init 5
  c:k init 3
  d:k init 7
  setp object, 2, d
  result:k run object, a, b, c
  printks "max's third input set to 7: %g\n", 0, result
  turnoff
endin

instr 6 ; oscil.kkk (kkjo) given four: two past the last
  object:Opcode create opcoderef("oscil"), 1
  amp:k init 1
  freq:k init 10
  setp object, 5, freq
  result:k run object, amp, freq, -1, 0
  turnoff
endin

instr 7 ; lincos given three: a string for its first input
  object:Opcode create opcoderef("lincos")
  x:k init 0.5
  y0:k init 0
  y1:k init 10
  setp object, 0, "text"
  result:k run object, x, y0, y1
  turnoff
endin

; An object kept in a global outlives the note whose call failed to bind it.
; That call bound the first input, then stopped at the second.
shared@global:Opcode = create(opcoderef("lincos"))

instr 8
  x:k init 0.5
  result:k init shared, x, "a string", x
  turnoff
endin

instr 9 ; the second input, which it left empty
  setp shared, 1, 0.5
  turnoff
endin

instr 10
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
i 9 0.8 0.1
i 10 0.9 0.1
e
</CsScore>
</CsoundSynthesizer>
