<CsTest>
description = "setp on an opcode object from its own run fails instead of deadlocking"

[expect]
exit = "nonzero"
stderr_regex = ["cannot set an opcode object from its own run"]
stderr_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode SelfSet(o:Opcode, x:k):k
  setp o, 0, 1
  xout 0
endop

self@global:Opcode = create(opcoderef("SelfSet"))

instr 1
  value:k run self, self, 0
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
