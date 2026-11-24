<CsTest>
description = "An Opcode[] whose objects take an array argument differently, one whole and one split into scalars, is refused at init instead of writing through an unallocated slot"

[expect]
exit = 2
stderr = ["has a different signature"]
output = ["same signature ran 3 3", "the next note ran"]
output_excludes = ["mixed ran", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Whole(values:k[]):k
  xout values[0]
endop

opcode Scalar(value:k):k
  xout value
endop

; the first object takes the array whole, the second as split scalars
instr 1
  objects:Opcode[] = create(opcoderef("Whole"), 2)
  other:Opcode = create(opcoderef("Scalar"))
  objects[1] = other
  values:k[] fillarray 3, 4
  outputs:k[] init 2
  outputs run objects, values
  printks "mixed ran %g %g\n", 0, outputs[0], outputs[1]
endin

; the same, the other way round
instr 3
  objects:Opcode[] = create(opcoderef("Scalar"), 2)
  other:Opcode = create(opcoderef("Whole"))
  objects[1] = other
  values:k[] fillarray 3, 4
  outputs:k[] init 2
  outputs run objects, values
  printks "mixed ran %g %g\n", 0, outputs[0], outputs[1]
endin

; objects of one signature still run
instr 4
  objects:Opcode[] = create(opcoderef("Whole"), 2)
  values:k[] fillarray 3, 4
  outputs:k[] init 2
  outputs run objects, values
  printks "same signature ran %g %g\n", 0, outputs[0], outputs[1]
endin

instr 2
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 4 0 0.1
i 1 0.1 0.1
i 3 0.2 0.1
i 2 0.3 0.1
e
</CsScore>
</CsoundSynthesizer>
