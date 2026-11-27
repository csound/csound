<CsTest>
description = "running an opcode object after its UDO creator note ends fails clearly"

[expect]
exit = "nonzero"
stderr_regex = ["INIT ERROR in instr 2 .*opcode object UDO instance has ended"]
stderr_excludes = ["AddressSanitizer", "Segmentation fault"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Twice(x:k):k
  xout x * 2
endop

opcode MakeObj(x:i):Opcode
  object:Opcode = create(opcoderef("Twice"))
  unused:k init object, 1
  xout object
endop

instr 1
  saved@global:Opcode = MakeObj(1)
endin

instr 2
  result:k run saved, 3
  printks "ended UDO result: %g\n", 1, result
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
i 2 0.1 0.1
e
</CsScore>
</CsoundSynthesizer>
