<CsTest>
description = "Copied UDO audio arrays preserve every sample through nesting, reinit and partial blocks"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 128
ksmps = 8
nchnls = 1
0dbfs = 256
gkNotesChecked init 0

opcode ClassicGain, a[], a[]i
  aInput[], iBlock xin
  setksmps iBlock
  aOffset[] init 2
  aOffset[0] = 1
  aOffset[1] = 2
  aResult[] = aInput + aOffset
  ; Changing the local copy must not change the caller's input.
  aInput[0] = -100
  xout aResult
endop

opcode ModernGain(aInput:a[], iBlock:i):a[]
  setksmps iBlock
  aOffset[] init 2
  aOffset[0] = 1
  aOffset[1] = 2
  aResult[] = aInput + aOffset
  aInput[0] = -100
  xout aResult
endop

opcode NestedGain, a[], a[]
  setksmps 4
  aInput[] xin
  ; Copy from eight samples to four, then to one, and back out again.
  aResult[] ClassicGain aInput, 1
  xout aResult
endop

instr CheckBlocks
  iStart = round(p2*sr)
  iEnd = iStart + round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kBlock init p4
  kCycle timeinstk
  if p5 == 1 && kCycle > 1 then
    ; Reuse the same calls with block sizes 8, 2, 1, 4, then 8 again.
    kSizes[] fillarray 8, 2, 1, 4, 8
    kBlock = kSizes[kCycle-1]
    reinit CHANGE_BLOCK
  endif

  aRamp phasor 1
  aInput[] init 2
  aInput[0] = 10 + aRamp
  aInput[1] = 20 + aRamp
  aInPlace[] = aInput
CHANGE_BLOCK:
  aClassic[] ClassicGain aInput, i(kBlock)
  aModern[] ModernGain aInput, i(kBlock)
  aInPlace[] ClassicGain aInPlace, i(kBlock)
  aNested[] NestedGain aInput
  rireturn

  kElement = 0
  while kElement < 2 do
    kSample = 0
    while kSample < ksmps do
      kActive = (kBlockStart+kSample >= iStart && kBlockStart+kSample < iEnd ? 1 : 0)
      kRamp vaget kSample, aRamp
      kInput vaget kSample, aInput[kElement]
      kExpectedInput = (10*(kElement+1)+kRamp)*kActive
      kExpected = (kExpectedInput+kElement+1)*kActive
      kClassic vaget kSample, aClassic[kElement]
      kModern vaget kSample, aModern[kElement]
      kInPlace vaget kSample, aInPlace[kElement]
      kNested vaget kSample, aNested[kElement]
      if (kActive == 1 && abs(kInput-kExpectedInput) > .00001) || abs(kClassic-kExpected) > .00001 || \
         abs(kModern-kExpected) > .00001 || abs(kInPlace-kExpected) > .00001 || \
         abs(kNested-kExpected) > .00001 then
        printks "block %g, element %g, sample %g expected %g, got classic %g, modern %g, in-place %g, nested %g; input %g\n", \
          0, kBlock, kElement, kSample, kExpected, kClassic, kModern, kInPlace, kNested, kInput
        exitnowk -1
      endif
      kSample += 1
    od
    kElement += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkNotesChecked += 1
  endif
  kBlockStart += ksmps
endin

instr CheckResults
  if i(gkNotesChecked) != 6 then
    prints "Expected six complete audio-array checks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Initial local block size, whether to change it during the note.
i "CheckBlocks" 0 .3125 8 1
i "CheckBlocks" .5 .125 1 0
i "CheckBlocks" .75 .125 2 0
i "CheckBlocks" 1 .125 4 0
; Start inside a block and cross several local blocks.
i "CheckBlocks" [1.25+3/128] [13/128] 1 0
i "CheckBlocks" [1.5+3/128] [13/128] 2 0
i "CheckResults" 2 .0625
</CsScore>
</CsoundSynthesizer>
