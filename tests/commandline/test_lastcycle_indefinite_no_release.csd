<CsTest>
description = "lastcycle does not add a release to an indefinite note"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; One control cycle is 16/1024 = 0.015625 seconds.
sr = 1024
ksmps = 16
nchnls = 1
gkCyclesRendered init 0

instr 1
  kBefore lastcycle
  xtratim 0
  kAfter lastcycle

  ; With no release, an external turnoff cannot be predicted.
  ; Both lastcycle calls must stay zero and must not prolong the note.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  if kBefore != 0 || kAfter != 0 then
    printks "indefinite turnoff: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, 0, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 2
  ; Allow a release if one exists; lastcycle must not create one.
  turnoff2 1.03, 4, 1
  turnoff
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkCyclesRendered) != 5 then
    prints "Expected %g rendered cycles, got %g\n", 5, i(gkCyclesRendered)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Instrument 2 runs after instrument 1 on the fifth cycle (index 4).
; The stop ends the note immediately, after five cycles and no release.
i1.03 .75 -1
i2 .8125 .01
i99 .9375 .001
e
</CsScore>
</CsoundSynthesizer>
