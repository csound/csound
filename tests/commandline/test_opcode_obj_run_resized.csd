<CsTest>
description = "An Opcode[] run whose array create resized after the run's init finds its slots by the count its init saw: fewer objects run on, more are refused under an array argument"

[expect]
exit = 2
stderr = ["opcode object array resized since init"]
output = ["fewer ran on 5", "kept printing|", "a whole array kept running|", "a whole array of 4 kept running|", "the next note ran"]
output_excludes = ["more ran on", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; run's init made a slot table for two objects, split x and results
; across them, and create then gave the array three, or one
instr 1
  objects:Opcode[] = create(opcoderef("lincos"), 2)
  x:k[] fillarray 0.5, 0.25
  y0:k init 0
  y1:k init 10
  results:k[] init 2
  results run objects, x, y0, y1
  objects = create(opcoderef("lincos"), p4)
  if p4 < 2 then
    printks "fewer ran on %d\n", 1, results[0]
  else
    printks "more ran on %d\n", 1, results[0]
  endif
endin

; no array argument
instr 3
  objects:Opcode[] = create(opcoderef("printks"), 2)
  run objects, "kept printing|\n", 1
  objects = create(opcoderef("printks"), 1)
endin

; an array each object takes whole, beside the split results
opcode Total(values:k[]):k
  xout sumarray(values)
endop

instr 4
  objects:Opcode[] = create(opcoderef("Total"), p5)
  values:k[] fillarray 1, 2
  results:k[] init p5
  results run objects, values
  objects = create(opcoderef("Total"), p4)
  if p5 == 4 then
    printks "a whole array of 4 kept running|\n", 1
  else
    printks "a whole array kept running|\n", 1
  endif
endin

instr 2
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1 3
i 1 0.1 0.1 1
i 3 0.2 0.1
i 4 0.3 0.1 1 2
i 4 0.4 0.1 3 2
i 4 0.5 0.1 1 4
i 2 0.6 0.1
e
</CsScore>
</CsoundSynthesizer>
