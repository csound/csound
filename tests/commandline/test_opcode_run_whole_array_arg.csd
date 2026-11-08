<CsTest>
description = "run on an opcode object array passes array parameters whole"

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
#include "libassert.orc"

opcode Tone(sum:a, gains:k[], f:i, n:i):a
  sum += oscili(gains[n], f)
  xout sum
endop

instr 1
  ref:OpcodeDef = opcoderef("Tone")
  objects:Opcode[] = create(ref, 2)
  gains:k[] fillarray 0.1, 0.2, 0.3
  freqs:i[] fillarray 220, 330
  indices:i[] fillarray 0, 1
  sum:a = 0
  sum = run(objects, sum, gains, freqs, indices)
  ; The whole-array parameter must keep its own length.
  assertEquals(lenarray(gains), 3)
  want:a = oscili(0.1, 220) + oscili(0.2, 330)
  assertSameSignal(sum, want, "whole-array parameter")
  assertNotSilent(want, "whole-array parameter")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
