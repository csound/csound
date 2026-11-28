<CsTest>
description = "copying a struct with an uninitialized Opcode member succeeds"

[expect]
exit = 0
output = ["empty struct copy ok"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

struct Holder item:Opcode

instr 1
  src@global:Holder init
  dst:Holder init src
  prints "empty struct copy ok\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
