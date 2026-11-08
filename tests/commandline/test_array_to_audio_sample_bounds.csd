<CsTest>
description = "a(k[]) preserves sample positions and clears inactive or missing samples"

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
gkChecks init 0

opcode LocalFour, a, k[]
  setksmps 4
  kSamples[] xin
  aResult = a(kSamples)
  xout aResult
endop

opcode LocalTwo, a, k[]
  setksmps 2
  kSamples[] xin
  aResult = a(kSamples)
  xout aResult
endop

instr CheckConversion
  iLength = p4
  ; Zero selects a direct call; other values select the UDO's block size.
  iLocalBlock = p5
  iBlockSize = (iLocalBlock == 0 ? ksmps : iLocalBlock)
  iStart = round(p2*sr)
  iEnd = iStart+round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kSamples[] init iLength
  kBase = 10*timeinstk()
  kIndex = 0
  while kIndex < iLength do
    kSamples[kIndex] = kBase+kIndex+1
    kIndex += 1
  od

  if iLocalBlock == 0 then
    aResult = a(kSamples)
  elseif iLocalBlock == 4 then
    aResult LocalFour kSamples
  else
    aResult LocalTwo kSamples
  endif

  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlockStart+kIndex
    kArrayIndex = kIndex % iBlockSize
    kExpected = 0
    if kSample >= iStart && kSample < iEnd && kArrayIndex < iLength then
      kExpected = kBase+kArrayIndex+1
    endif
    kActual vaget kIndex, aResult
    if kActual != kExpected then
      printks "a(k[]): length %g, local block %g, sample %g: expected %g, got %g\n", \
        0, iLength, iLocalBlock, kSample, kExpected, kActual
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
  if i(gkChecks) != 10 then
    prints "Array-to-audio checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Empty, short, exact and longer-than-block arrays.
i "CheckConversion" 0 [16/32] 0 0
i "CheckConversion" 0 [16/32] 3 0
i "CheckConversion" 0 [16/32] 8 0
i "CheckConversion" 0 [16/32] 10 0
; Start at sample 3 of a block and end at sample 7 of a later block.
i "CheckConversion" [19/32] [20/32] 0 0
i "CheckConversion" [19/32] [20/32] 3 0
i "CheckConversion" [19/32] [20/32] 8 0
i "CheckConversion" [19/32] [20/32] 10 0
; Local blocks use their own sample positions and note boundaries.
i "CheckConversion" [43/32] [12/32] 3 4
i "CheckConversion" [43/32] [12/32] 8 2
i "CheckResults" 2 .25
e
</CsScore>
</CsoundSynthesizer>
