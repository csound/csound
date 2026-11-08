<CsTest>
description = "perf on a native opcode object follows its current arguments"

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

instr 1
  initAmp:k init 0.5
  initFreq:k init 110
  amp:k init 0.2
  freq:k init 440
  osc:Opcode = create(oscili)
  sig:a init 0
  ; init binds other variables than perf
  sig init osc, initAmp, initFreq
  ; every perf passes the same variables, whose values change
  amp = 0.2 + 0.1 * timeinsts()
  freq = 440 + 100 * timeinsts()
  sig perf osc, amp, freq
  assertSameSignal(sig, oscili(amp, freq), "native object")
  assertNotSilent(sig, "native object")
  delete(osc)
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
i 1 0.3 0.2
e
</CsScore>
</CsoundSynthesizer>
