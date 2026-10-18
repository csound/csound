<CsTest>
description = "instanced loop body can write several outer variables"

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
  sum:a = 0
  loudest:k = 0
  voices:k = 0
  for f:i in freqs instanced do
    voice:a = oscili(0.1 + f / 4400, f)
    sum += voice
    loudest = max(loudest, max_k(voice, 1, 1))
    voices += 1
  od
  want:a = oscili(0.1 + 220 / 4400, 220) + oscili(0.1 + 330 / 4400, 330) + oscili(0.1 + 440 / 4400, 440)
  assertSameSignal(sum, want, "several outputs")
  assertNotSilent(want, "several outputs")
  if voices != 3 then
    printks("several outputs: counted %d voices\n", 0, voices)
    exitnowk(-1)
  endif
  ; A cycle is shorter than a period, so check the loudest value of the note.
  best:k init 0
  best = max(best, loudest)
  if lastcycle() == 1 && best < 0.19 then
    printks("several outputs: loudest voice %f\n", 0, best)
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
