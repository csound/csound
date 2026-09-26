<CsTest>
description = "Audio-array arithmetic processes every matrix element and clears inactive samples"

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
gaInput[][] init 2, 2
gkChecks init 0

instr FeedInput
  ; Supply nonzero samples even outside the checked note's active interval.
  gaInput[0][0] = 1
  gaInput[0][1] = 2
  gaInput[1][0] = 3
  gaInput[1][1] = 4
endin

instr CheckMatrix
  iStart = round(p2*sr)
  iEnd = iStart+round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  aSum[][] = gaInput + gaInput
  aShifted[][] = gaInput + 10
  aScaled[][] = 2 * gaInput
  aCompound[][] = gaInput * 2
  aCompound += gaInput

  kRow = 0
  while kRow < 2 do
    kColumn = 0
    while kColumn < 2 do
      kInput = 1+2*kRow+kColumn
      kIndex = 0
      while kIndex < ksmps do
        kSample = kBlockStart+kIndex
        kActive = (kSample >= iStart && kSample < iEnd ? 1 : 0)
        kSum vaget kIndex, aSum[kRow][kColumn]
        kShifted vaget kIndex, aShifted[kRow][kColumn]
        kScaled vaget kIndex, aScaled[kRow][kColumn]
        kCompound vaget kIndex, aCompound[kRow][kColumn]
        if kSum != 2*kInput*kActive || \
           kShifted != (kInput+10)*kActive || \
           kScaled != 2*kInput*kActive || kCompound != 3*kInput*kActive then
          printks "Matrix [%g][%g], sample %g, active %g: got sum=%g shifted=%g scaled=%g compound=%g\n", \
            0, kRow, kColumn, kSample, kActive, kSum, kShifted, kScaled, kCompound
          exitnowk -1
        endif
        kIndex += 1
      od
      kColumn += 1
    od
    kRow += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "Audio-array matrix checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "FeedInput" 0 2
i "CheckMatrix" 0 [16/32]
; Start three samples into a block and stop one sample before its end.
i "CheckMatrix" [35/32] [20/32]
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
