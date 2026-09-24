<CsTest>
description = "trighold expiry works across full and partial audio blocks"

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

instr CheckAudioBlocks
  iPeriodSamples = p4
  iHoldSamples = p5
  aTrigger mpulse .5, -iPeriodSamples
  aHeld trighold aTrigger, iHoldSamples/sr
  aActive = 1
  kElapsedSamples init 0
  kBlockSample = 0
  while kBlockSample < ksmps do
    kActive vaget kBlockSample, aActive
    kHeld vaget kBlockSample, aHeld
    kExpected = 0
    if kActive != 0 then
      kAge = kElapsedSamples % iPeriodSamples
      kExpected = kAge < iHoldSamples ? .5 : 0
      kElapsedSamples += 1
    endif
    if kHeld != kExpected then
      printks "trighold period=%g hold=%g block sample=%g expected=%g actual=%g\n", \
        0, iPeriodSamples, iHoldSamples, kBlockSample, kExpected, kHeld
      exitnowk(-1)
    endif
    kBlockSample += 1
  od
  if kElapsedSamples == 48 then
    gkChecks += 1
  endif
endin

instr CheckCoverage
  if i(gkChecks) != 4 then
    prints "trighold completed %g block cases; expected 4\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
;                                            period hold (samples)
; Adjacent holds expire inside a block and exactly at its boundary.
i "CheckAudioBlocks" 0            [48/8192]     3     3
i "CheckAudioBlocks" [64/8192]    [48/8192]    16    16
; Start three samples into a block: the first hold expires at the next block.
i "CheckAudioBlocks" [(128+3)/8192] [48/8192]  13    13
; A two-sample gap between holds must stay silent, as must inactive samples.
i "CheckAudioBlocks" [(192+3)/8192] [48/8192]   5     3
i "CheckCoverage" [256/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
