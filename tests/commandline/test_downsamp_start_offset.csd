<CsTest>
description = "downsamp averages active samples after a note starts within a block"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
gkFailures init 0

instr CheckConstantAverage
  ; Every block has at least eleven active samples, so a four-sample
  ; average of this constant signal must equal the unaveraged value.
  aConstant = .75
  kFirstSample downsamp aConstant
  kAverage downsamp aConstant, 4
  kBlock init 0
  if kFirstSample != .75 || kAverage != .75 then
    printks "downsamp start=%g block=%g expected=.75 first=%g average=%g\n", \
      0, p2*sr, kBlock, kFirstSample, kAverage
    gkFailures += 1
  endif
  kBlock += 1
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 6 || i(gkFailures) != 0 then
    prints "downsamp checked %g blocks, with %g failures\n", i(gkChecks), i(gkFailures)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Each note ends at a block boundary and spans two performance calls.
i "CheckConstantAverage" 0 [32/8192]
i "CheckConstantAverage" [(48+3)/8192] [29/8192]
i "CheckConstantAverage" [(96+5)/8192] [27/8192]
i "CheckResults" [144/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
