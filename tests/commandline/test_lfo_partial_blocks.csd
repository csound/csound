<CsTest>
description = "lfo advances only through active samples in partial audio blocks"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckActiveSamples
  iStart = round(p2*sr)
  iEnd = iStart + round(p3*sr)
  iTwoPi = 8*taninv(1)
  kBlockStart init floor(iStart/ksmps)*ksmps
  aSaw lfo 1, sr*3/16, 4
  aSine lfo 1, sr*3/16, 0

  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlockStart+kIndex
    if kSample >= iStart && kSample < iEnd then
      ; Phase zero belongs to the first active sample, even mid-block.
      kPhase = (kSample-iStart)*3/16
      kExpectedSaw = kPhase-floor(kPhase)
      kExpectedSine = sin(iTwoPi*kExpectedSaw)
    else
      kExpectedSaw = 0
      kExpectedSine = 0
    endif
    kSaw vaget kIndex, aSaw
    kSine vaget kIndex, aSine
    if !(abs(kSaw-kExpectedSaw) < .000001 && abs(kSine-kExpectedSine) < .000001) then
      printks "lfo start=%g sample=%g expected=(%g,%g) actual=(%g,%g)\n", \
        0, iStart, kSample, kExpectedSaw, kExpectedSine, kSaw, kSine
      exitnowk(-1)
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
    prints "lfo completed %g block checks; expected 2\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, then a note that starts three samples into a block.
i "CheckActiveSamples" 0 [64/32768]
; Its final block ends one sample early; both inactive regions must be zero.
i "CheckActiveSamples" [83/32768] [60/32768]
i "CheckResults" [160/32768] [16/32768]
e
</CsScore>
</CsoundSynthesizer>
