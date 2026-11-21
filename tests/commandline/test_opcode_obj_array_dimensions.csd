<CsTest>
description = "An Opcode object takes an input or an output that is an array of two dimensions, and setp none past its inputs"

[expect]
exit = 1
stderr = ["could not set arg 6"]
output = ["Grid: 15", "dbapgains: 4 gains", "savgolmat: 3 by 5", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; Grid's inputs are k[][]k: the second [] belongs to the first input.
opcode Grid(values:k[][], scale:k):k
  xout lenarray(values, 2) * scale
endop

instr 1
  object:Opcode create opcoderef("Grid")
  values:k[][] init 2, 3
  scale:k init 5
  result:k run object, values, scale
  printks "Grid: %g\n", 0, result
  turnoff
endin

; dbapgains' first overload takes ik[]i[][]kii[], six inputs.
instr 2
  object:Opcode create opcoderef("dbapgains"), 0
  source:k[] fillarray 0, 0
  positions:i[][] init 4, 2
  positions = [0, 0, 90, 0, 180, 0, 270, 0]
  weights:i[] fillarray 1, 1, 1, 1
  spread:k init 0
  gains:k[] init 4
  gains run object, 1, source, positions, spread, 6, weights
  printks "dbapgains: %d gains\n", 0, lenarray(gains)
  setp object, 6, spread
  turnoff
endin

; savgolmat.i's output is i[][].
instr 3
  object:Opcode create opcoderef("savgolmat")
  matrix:i[][] init 1, 1
  matrix init object, 5, 2
  prints "savgolmat: %d by %d\n", lenarray(matrix, 1), lenarray(matrix, 2)
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
