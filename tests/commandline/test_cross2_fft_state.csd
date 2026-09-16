<CsTest>
description = "cross2 rounded FFT size and overlap bounds"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
giwin ftgen 1, 0, 1024, 20, 2
gkchecks init 0

instr 1
  a1 oscili .2, 440
  a2 oscili .2, 660
  aRound cross2 a1, a2, 1000, 4, giwin, 1
  aPower cross2 a1, a2, 1024, 4, giwin, 1
  kDiff max_k abs(aRound - aPower), 1, 1
  if timeinsts() > .08 && kDiff > .000001 then
    printks "FAIL cross2 non-power-of-two length differs by %g\n", 0, kDiff
    exitnowk -1
  endif
  if timeinsts() > .19 then
    gkchecks += 1
    turnoff
  endif
endin

instr 2
  a1 oscili .2, 330
  a2 oscili .2, 550
  aClamp cross2 a1, a2, 16, 128, giwin, 1
  aLimit cross2 a1, a2, 16, 16, giwin, 1
  kDiff max_k abs(aClamp - aLimit), 1, 1
  if timeinsts() > .02 && kDiff > .000001 then
    printks "FAIL cross2 clamped overlap differs by %g\n", 0, kDiff
    exitnowk -1
  endif
  if timeinsts() > .09 then
    gkchecks += 1
    turnoff
  endif
endin

instr 3
  a1 oscili .2, 220
  a2 oscili .2, 440
  aSmall cross2 a1, a2, 1, 2, giwin, 1
  aMinimum cross2 a1, a2, 8, 2, giwin, 1
  kDiff max_k abs(aSmall - aMinimum), 1, 1
  if timeinsts() > .02 && kDiff > .000001 then
    printks "FAIL cross2 minimum FFT size differs by %g\n", 0, kDiff
    exitnowk -1
  endif
  if timeinsts() > .09 then
    gkchecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkchecks) != 3 then
    prints "FAIL cross2 checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .2
i 2 .21 .1
i 3 .32 .1
i 99 .43 .001
</CsScore>
</CsoundSynthesizer>
