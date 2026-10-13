<CsTest>
description = "Audio-array taninv2 clears inactive samples in each array element"

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
gaY[] init 2
gaX[] init 2
gkChecks init 0

instr FeedInput
  ; A separate note supplies nonzero input across each whole block.
  gaY[0] = 1
  gaX[0] = 1
  gaY[1] = -1
  gaX[1] = 1
endin

instr CheckActiveSamples
  iStart = round(p2*sr)
  iEnd = iStart+round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  aAngles[] = taninv2(gaY, gaX)
  kElement = 0
  while kElement < 2 do
    kIndex = 0
    while kIndex < ksmps do
      kSample = kBlockStart+kIndex
      kExpected = 0
      if kSample >= iStart && kSample < iEnd then
        kExpected = (kElement == 0 ? 1 : -1)*taninv2(1, 1)
      endif
      kActual vaget kIndex, aAngles[kElement]
      if !(abs(kActual-kExpected) < .000001) then
        printks "taninv2 element %g sample %g: expected %g, got %g\n", \
          0, kElement, kSample, kExpected, kActual
        exitnowk -1
      endif
      kIndex += 1
    od
    kElement += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "taninv2 partial-block checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "FeedInput" 0 [160/32768]
i "CheckActiveSamples" 0 [64/32768]
; Begin three samples into a block and end one sample before a block boundary.
i "CheckActiveSamples" [83/32768] [60/32768]
i "CheckResults" [160/32768] [16/32768]
e
</CsScore>
</CsoundSynthesizer>
