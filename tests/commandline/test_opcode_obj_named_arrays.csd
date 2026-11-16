<CsTest>
description = "An Opcode object takes an input or an output that is an array of a type with a long name, and refuses an array of another type"

[expect]
exit = 2
stderr = ["mismatching arguments"]
output = ["Count: plain 3 object 3", "Size: plain 2 object 2", "Make: 2", "Make outarg 1, mismatching array subtype expected Instr, got k", "Count inarg 1, mismatching array subtype expected Instr, got k", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

struct Point x:k, y:k

instr Voice
endin

opcode Count(voices:Instr[]):k
  xout lenarray(voices)
endop

opcode Size(points:Point[]):k
  xout lenarray(points)
endop

opcode Make(n:i):Instr[]
  voices:Instr[] init n
  xout voices
endop

instr 1
  voices:Instr[] init 3
  plain:k = Count(voices)
  object:Opcode create opcoderef("Count")
  result:k run object, voices
  printks "Count: plain %d object %d\n", 0, plain, result
  turnoff
endin

instr 2
  points:Point[] init 2
  plain:k = Size(points)
  object:Opcode create opcoderef("Size")
  result:k run object, points
  printks "Size: plain %d object %d\n", 0, plain, result
  turnoff
endin

instr 3
  object:Opcode create opcoderef("Make")
  voices:Instr[] init 1
  voices init object, 2
  prints "Make: %d\n", lenarray(voices)
  turnoff
endin

; a call of Make into a k[] does not compile
instr 4
  object:Opcode create opcoderef("Make")
  values:k[] init 1
  values init object, 2
  turnoff
endin

instr 5
  object:Opcode create opcoderef("Count")
  values:k[] init 3
  result:k run object, values
  turnoff
endin

instr 6
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
e
</CsScore>
</CsoundSynthesizer>
