<CsTest>
description = "An Opcode object takes an opcode whose input is an array of any type, such as lenarray, refuses a number for that array, and still refuses an output of any element type"

[expect]
exit = 3
stderr = ["mismatching arguments", "could not set arg 2"]
output = [
  "mismatching array subtype",
  "lenarray: 5",
  "lenarray, first dimension: 5",
  "lenarray, second dimension: 3",
  "lenarray of strings: 4",
  "expect array",
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

; lenarray.kx takes .[]p: an array of any type, and the dimension to measure.

instr 1
  object:Opcode create opcoderef("lenarray"), 1
  values:k[] init 5
  result:k run object, values
  printks "lenarray: %g\n", 0, result
  turnoff
endin

instr 2
  object:Opcode create opcoderef("lenarray"), 1
  values:k[] init 5
  result:k run object, values, 1
  printks "lenarray, first dimension: %g\n", 0, result
  turnoff
endin

instr 3
  object:Opcode create opcoderef("lenarray"), 1
  values:k[][] init 2, 3
  result:k run object, values, 2
  printks "lenarray, second dimension: %g\n", 0, result
  turnoff
endin

instr 4
  object:Opcode create opcoderef("lenarray"), 1
  names:S[] init 4
  result:k run object, names
  printks "lenarray of strings: %g\n", 0, result
  turnoff
endin

instr 5 ; a number is no array
  object:Opcode create opcoderef("lenarray"), 1
  value:k init 5
  result:k run object, value
  turnoff
endin

instr 6 ; the array and its defaulted dimension are the only inputs
  object:Opcode create opcoderef("lenarray"), 1
  values:k[] init 5
  setp object, 1, 1
  setp object, 2, 1
  result:k run object, values
  turnoff
endin

; An output of any element type stays refused: fillarray. (overload 10)
; relies on the compiler to match its output to its inputs, and an object
; checks each argument on its own, so it would copy numbers as strings.
instr 7
  object:Opcode create opcoderef("fillarray"), 10
  names:S[] init 1
  names init object, 1, 2, 3
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
