<CsTest>
description = "downsamp selects and averages the requested active samples"

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

instr CheckRampWindow
  iWindowSamples = p4
  aActive = 1
  aRamp init 0
  kNextValue init 1
  kFirstValue = kNextValue
  kActiveSamples = 0
  kSample = 0
  while kSample < ksmps do
    kActive vaget kSample, aActive
    if kActive != 0 then
      vaset kNextValue, kSample, aRamp
      kNextValue += 1
      kActiveSamples += 1
    else
      ; Inactive samples must never contribute to the result.
      vaset -1000, kSample, aRamp
    endif
    kSample += 1
  od

  ; The active input is 1, 2, 3, ... across successive blocks.
  ; Its first N samples have mean first_value + (N-1)/2.
  ; Zero and one select the first sample without averaging.
  kUsedSamples = min(max(int(iWindowSamples), 1), kActiveSamples)
  kExpected = kFirstValue + (kUsedSamples-1)/2
  kAverage downsamp aRamp, iWindowSamples
  if kAverage != kExpected then
    printks "downsamp start=%g window=%g first=%g active=%g expected=%g actual=%g\n", \
      0, p2*sr, iWindowSamples, kFirstValue, kActiveSamples, kExpected, kAverage
    exitnowk(-1)
  endif
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 40 then
    prints "downsamp checked %g windows; expected 40\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks: retain first-sample selection and window-length truncation.
i "CheckRampWindow" 0 [32/8192] 0
i "CheckRampWindow" 0 [32/8192] 1
i "CheckRampWindow" 0 [32/8192] 4
i "CheckRampWindow" 0 [32/8192] 4.75
i "CheckRampWindow" 0 [32/8192] 16
; A late start still leaves room for a four-sample window.
i "CheckRampWindow" [(48+3)/8192] [29/8192] 0
i "CheckRampWindow" [(48+3)/8192] [29/8192] 1
i "CheckRampWindow" [(48+3)/8192] [29/8192] 4
i "CheckRampWindow" [(48+3)/8192] [29/8192] 4.75
i "CheckRampWindow" [(48+3)/8192] [29/8192] 16
; One active sample at the end of a block must not be attenuated.
i "CheckRampWindow" [(96+15)/8192] [1/8192] 0
i "CheckRampWindow" [(96+15)/8192] [1/8192] 1
i "CheckRampWindow" [(96+15)/8192] [1/8192] 4
i "CheckRampWindow" [(96+15)/8192] [1/8192] 4.75
i "CheckRampWindow" [(96+15)/8192] [1/8192] 16
; A short note starts and ends inside the same block.
i "CheckRampWindow" [(144+5)/8192] [3/8192] 0
i "CheckRampWindow" [(144+5)/8192] [3/8192] 1
i "CheckRampWindow" [(144+5)/8192] [3/8192] 4
i "CheckRampWindow" [(144+5)/8192] [3/8192] 4.75
i "CheckRampWindow" [(144+5)/8192] [3/8192] 16
; The final block has only three active samples: divide by three.
i "CheckRampWindow" [192/8192] [19/8192] 0
i "CheckRampWindow" [192/8192] [19/8192] 1
i "CheckRampWindow" [192/8192] [19/8192] 4
i "CheckRampWindow" [192/8192] [19/8192] 4.75
i "CheckRampWindow" [192/8192] [19/8192] 16
i "CheckResults" [240/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
