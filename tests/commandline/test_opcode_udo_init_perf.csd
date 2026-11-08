<CsTest>
description = "init and perf on single UDO opcode objects bind the arguments of each call"

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

opcode Voice(f:k):a
  sig:a = oscili(0.25, f)
  xout sig
endop

opcode Accumulate(sum:a, f:k):a
  sum += oscili(0.25, f)
  xout sum
endop

opcode Outer(f:k):a
  sig:a = Voice(f)
  xout sig
endop

instr 1
  freqs:i[] fillarray 440, 660
  voices:Opcode[] = create(opcoderef("Voice"), 2)
  adders:Opcode[] = create(opcoderef("Accumulate"), 2)
  nested:Opcode[] = create(opcoderef("Outer"), 2)
  sigs:a[] init 2
  acc:a = 0
  outs:a[] init 2
  n:i = 0
  while n < 2 do
    ; init and perf receive different argument variables
    sigs[n] init voices[n], freqs[n]
    acc init adders[n], acc, freqs[n]
    outs[n] init nested[n], freqs[n]
    n += 1
  od

  sum:a = 0
  acc = 0
  nestedSum:a = 0
  k:k = 0
  while k < 2 do
    sig:a perf voices[k], freqs[k]
    sum += sig
    acc perf adders[k], acc, freqs[k]
    outs[k] perf nested[k], freqs[k]
    nestedSum += outs[k]
    k += 1
  od

  want:a = oscili(0.25, 440) + oscili(0.25, 660)
  assertSameSignal(sum, want, "init/perf")
  assertSameSignal(acc, want, "pass-through argument")
  assertSameSignal(nestedSum, want, "nested UDO")
  assertNotSilent(sum, "init/perf")
  delete(voices)
  delete(adders)
  delete(nested)
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
i 1 0.3 0.2
e
</CsScore>
</CsoundSynthesizer>
