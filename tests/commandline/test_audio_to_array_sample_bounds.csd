<CsTest>
description = "array(a) and audio-to-array assignment clear inactive samples"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 32
gaInput init 0
gkChecks init 0

instr FeedInput
  ; A global signal has nonzero samples outside the checked note's interval.
  kSamples[] fillarray 1, 2, 3, 4, 5, 6, 7, 8
  gaInput = a(kSamples)
endin

instr CheckConversion
  iStart = round(p2*sr)
  iEnd = iStart+round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kConverted[] = array(gaInput)
  kAssigned[] = gaInput
  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlockStart+kIndex
    kExpected = 0
    if kSample >= iStart && kSample < iEnd then
      kExpected = kIndex+1
    endif
    if kConverted[kIndex] != kExpected || kAssigned[kIndex] != kExpected then
      printks "Audio-to-array sample %g: expected %g, got array(a)=%g, assignment=%g\n", \
        0, kSample, kExpected, kConverted[kIndex], kAssigned[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "Audio-to-array sample checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "FeedInput" 0 2
i "CheckConversion" 0 [16/32]
i "CheckConversion" [19/32] [20/32]
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
