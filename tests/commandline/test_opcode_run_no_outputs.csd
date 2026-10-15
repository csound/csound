<CsTest>
description = "run on opcode objects without outputs finds its object and inputs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

#include "loops/libinstanced.orc"

bus@global:a init 0

opcode AddTone(f:i):void
  bus += oscili(0.2, f)
endop

instr 1
  bus = 0
  ref:OpcodeDef = opcoderef("AddTone")
  single:Opcode = create(ref)
  run(single, 220)
  many:Opcode[] = create(ref, 2)
  freqs:i[] fillarray 330, 440
  run(many, freqs)
  want:a = oscili(0.2, 220) + oscili(0.2, 330) + oscili(0.2, 440)
  assertSameSignal(bus, want, "run without outputs")
  assertNotSilent(want, "run without outputs")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
