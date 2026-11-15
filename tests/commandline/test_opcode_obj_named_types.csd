<CsTest>
description = "An Opcode object takes an input or an output of a struct type, and refuses a type whose name only starts with the one declared"

[expect]
exit = 2
stderr = ["mismatching arguments"]
output = ["Px: plain 7 object 7", "MakeP: 5 10", "Named: 3", "Use inarg 1, expect type: Opcode, got OpcodeDef", "MakeObj outarg 1, expect type: Opcode, got OpcodeDef", "the next note ran"]
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

; a struct type is named :Point; inside, where Px's intypes say Point
opcode Px(point:Point):k
  xout point.x + point.y
endop

opcode MakeP(x:k):Point
  point:Point init 0, 0
  point.x = x
  point.y = x * 2
  xout point
endop

opcode Named(definition:OpcodeDef, value:k):k
  xout value
endop

opcode Use(object:Opcode):k
  xout 1
endop

opcode MakeObj(x:i):Opcode
  object:Opcode = create(opcoderef("oscil"))
  xout object
endop

instr 1
  point:Point init 3, 4
  plain:k = Px(point)
  object:Opcode create opcoderef("Px")
  result:k run object, point
  printks "Px: plain %d object %d\n", 0, plain, result
  turnoff
endin

instr 2
  object:Opcode create opcoderef("MakeP")
  result:Point init 0, 0
  result run object, 5
  printks "MakeP: %d %d\n", 0, result.x, result.y
  turnoff
endin

; a builtin long name binds as before
instr 3
  object:Opcode create opcoderef("Named")
  result:k run object, opcoderef("oscil"), 3
  printks "Named: %d\n", 0, result
  turnoff
endin

; OpcodeDef starts with Opcode, and a call of Use(definition) does not compile
instr 4
  definition:OpcodeDef = opcoderef("oscil")
  object:Opcode create opcoderef("Use")
  result:k run object, definition
  turnoff
endin

instr 5
  object:Opcode create opcoderef("MakeObj")
  definition:OpcodeDef = opcoderef("lincos")
  definition init object, 1
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
