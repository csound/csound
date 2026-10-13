<CsTest>
description = "balance, balance2, and gain retain level matching at low smoothing cutoffs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckLevelMatching
  iCutoff = p4
  iStart = round(p2*sr)
  iEnd = iStart + round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kFirst init 1
  aInput = 1
  aReference = .5
  aBalanced balance aInput, aReference, iCutoff
  aBalancedPerSample balance2 aInput, aReference, iCutoff
  kReferenceRms rms aReference, iCutoff
  aGain gain aInput, kReferenceRms, iCutoff

  kActiveStart = max(iStart, kBlockStart)
  kActiveEnd = min(iEnd, kBlockStart+ksmps)
  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlockStart+kIndex
    kExpected = 0
    kExpectedPerSample = 0
    if kSample >= kActiveStart && kSample < kActiveEnd then
      ; The RMS ratio is exactly one half, regardless of smoothing cutoff.
      kExpectedPerSample = .5
      kExpected = .5
      if kFirst == 1 then
        ; balance and gain ramp from zero during their first active block.
        kExpected *= (kSample-kActiveStart)/(kActiveEnd-kActiveStart)
      endif
    endif
    kBalanced vaget kIndex, aBalanced
    kBalancedPerSample vaget kIndex, aBalancedPerSample
    kGain vaget kIndex, aGain
    if !(abs(kBalanced-kExpected) < .000001 && abs(kGain-kExpected) < .000001 && abs(kBalancedPerSample-kExpectedPerSample) < .000001) then
      printks "cutoff=%g start=%g sample=%g expected=(%g,%g) balance=%g gain=%g balance2=%g\n", \
        0, iCutoff, iStart, kSample, kExpected, kExpectedPerSample, kBalanced, kGain, kBalancedPerSample
      exitnowk(-1)
    endif
    kIndex += 1
  od
  if kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kFirst = 0
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkChecks) != 4 then
    prints "level matching completed %g cases; expected 4\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Ordinary and low smoothing cutoffs, with full and partial audio blocks.
i "CheckLevelMatching" 0 [64/48000] 100
i "CheckLevelMatching" 0 [64/48000] .00001
i "CheckLevelMatching" [83/48000] [60/48000] 100
i "CheckLevelMatching" [83/48000] [60/48000] .00001
i "CheckResults" [160/48000] [16/48000]
e
</CsScore>
</CsoundSynthesizer>
