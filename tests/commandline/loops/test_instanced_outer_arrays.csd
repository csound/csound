<CsTest>
description = "instanced loop body reads and writes outer arrays"

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

#include "libinstanced.orc"

instr 1
  freqs:i[] fillarray 220, 330, 440
  gains:k[] fillarray 0.1, 0.2, 0.3
  peaks:k[] init 3
  sum:a = 0
  for f:i, n:i in freqs instanced do
    voice:a = oscili(gains[n], f)
    peaks[n] = max_k(voice, 1, 1)
    sum += voice
  od
  want:a = oscili(0.1, 220) + oscili(0.2, 330) + oscili(0.3, 440)
  assertSameSignal(sum, want, "outer arrays")
  assertNotSilent(want, "outer arrays")
  ; Every voice reported its own peak, so the loudest is the last one.
  if timeinstk() > 200 && (peaks[2] <= peaks[1] || peaks[1] <= peaks[0]) then
    printks("outer arrays: peaks not written per iteration: %f %f %f\n", 0,
            peaks[0], peaks[1], peaks[2])
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
