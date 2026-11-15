<CsTest>
description = "An Opcode object given a scalar where its opcode takes an array names the type it was given"

[expect]
exit = 4
stderr = ["mismatching arguments"]
output = ["Count inarg 1, expect array, got k", "Make outarg 1, expect array, got k", "Sum inarg 1, expect array, got a", "MakeK outarg 1, expect array, got S", "the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Count(voices:Instr[]):k
  xout lenarray(voices)
endop

opcode Make(n:i):Instr[]
  voices:Instr[] init n
  xout voices
endop

opcode Sum(values:k[]):k
  xout sumarray(values)
endop

opcode MakeK(n:i):k[]
  values:k[] init n
  xout values
endop

instr 1
  object:Opcode create opcoderef("Count")
  value:k init 3
  result:k run object, value
  turnoff
endin

instr 2
  object:Opcode create opcoderef("Make")
  value:k init 0
  value init object, 2
  turnoff
endin

instr 3
  object:Opcode create opcoderef("Sum")
  sig:a = 0
  result:k run object, sig
  turnoff
endin

instr 4
  object:Opcode create opcoderef("MakeK")
  text:S init ""
  text init object, 2
  turnoff
endin

instr 5
  prints "the next note ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.1 0.1
i 3 0.2 0.1
i 4 0.3 0.1
i 5 0.4 0.1
e
</CsScore>
</CsoundSynthesizer>
