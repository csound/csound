<CsTest>
description = "vdelayk keeps skipped state and bounded delay wrapping"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kCount init 0
  if kCount == 11 || kCount == 23 then
    reinit DELAY
  endif
DELAY:
  ; A skipped reinit must ignore a changed maximum and keep the write head.
  iMaximum = (i(kCount) == 0 ? 8 : (i(kCount) == 11 ? 100 : 2)) / kr
  iSkip = i(kCount) != 0 ? 1 : 0
  kLinear vdelayk kCount, 1.5/kr, iMaximum, iSkip
  kDiscrete vdelayk kCount, 1.5/kr, iMaximum, iSkip, 1
  rireturn
  kNegative vdelayk kCount, -6.5/kr, 8/kr
  kLarge vdelayk kCount, 1099511627776/kr, 8/kr
  kMaximum vdelayk kCount, 8/kr, 8/kr
  kZero vdelayk kCount, 0, 0
  kExpected = (max(kCount-2,0) + max(kCount-1,0)) / 2
  if abs(kLinear-kExpected) > .00001 || abs(kNegative-kExpected) > .00001 || kDiscrete != max(kCount-2,0) || kLarge != kCount || kMaximum != kCount || kZero != kCount then
    printks "vdelayk mismatch at cycle %g\n", 0, kCount
    exitnowk(-1)
  endif
  if kCount == 40 then
    gkChecks += 1
  endif
  kCount += 1
endin

instr 99
  if i(gkChecks) != 1 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1
i 99 1 .1
</CsScore>
</CsoundSynthesizer>
