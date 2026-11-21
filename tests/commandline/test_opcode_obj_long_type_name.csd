<CsTest>
description = "An Opcode object takes an input and an output whose struct type has a name of more than 63 characters"

[expect]
exit = 0
output = ["Sum: plain 7 object 7", "Make: 5 10"]
output_excludes = ["stack smashing", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

; the type's name is 85 characters long
struct PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong x:k, y:k

opcode Sum(point:PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong):k
  xout point.x + point.y
endop

opcode Make(x:k):PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong
  point:PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong init 0, 0
  point.x = x
  point.y = x * 2
  xout point
endop

instr 1
  point:PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong init 3, 4
  plain:k = Sum(point)
  object:Opcode create opcoderef("Sum")
  result:k run object, point
  printks "Sum: plain %d object %d\n", 0, plain, result
  turnoff
endin

instr 2
  object:Opcode create opcoderef("Make")
  result:PointLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong init 0, 0
  result run object, 5
  printks "Make: %d %d\n", 0, result.x, result.y
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
e
</CsScore>
</CsoundSynthesizer>
