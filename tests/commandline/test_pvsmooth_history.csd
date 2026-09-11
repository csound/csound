<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

; The control-rate bin reader at ksmps=1 exposes every sample for the reference.
opcode SmoothReference, aa, aaaai
  setksmps 1
  aInput, aACut, aFCut, aReset, iBin xin
  kACut downsamp aACut
  kFCut downsamp aFCut
  kReset downsamp aReset
  fInput pvsanal aInput, 64, 1, 64, 1
  kInputAmp, kInputFreq pvsbin fInput, iBin
  if kReset != 0 then
    reinit RESET
  endif
RESET:
  kAmpHistory init 0
  kFreqHistory init 0
  rireturn
  kACut limit kACut, 0, 1
  kFCut limit kFCut, 0, 1
  kACosth = 2 - cos($M_PI * kACut)
  kFCosth = 2 - cos($M_PI * kFCut)
  kAB = kACosth - sqrt(kACosth * kACosth - 1)
  kFB = kFCosth - sqrt(kFCosth * kFCosth - 1)
  kAmpHistory = (1 - kAB) * kInputAmp + kAB * kAmpHistory
  kFreqHistory = (1 - kFB) * kInputFreq + kFB * kFreqHistory
  aAmp = kAmpHistory
  aFreq = kFreqHistory
  xout aAmp, aFreq
endop

instr 1
  kCycle init 0
  kCycle += 1
  aTone oscili .25, 3072
  aInput = 1 + aTone
  fInput pvsanal aInput, 64, 1, 64, 1
  aPhase phasor 512
  aAmpCut = 1.5 * aPhase - .25
  aFreqCut = 1.25 - 1.5 * aPhase
  kAmpCut = (kCycle < 5 ? .1 : .3)
  kFreqCut = (kCycle < 5 ? .2 : .4)
  aReset = 0
  if kCycle == p9 then
    vaset 1, 0, aReset
    reinit SMOOTH
  endif
SMOOTH:
  if p4 == 0 then
    fSmooth pvsmooth fInput, kAmpCut, kFreqCut
  elseif p4 == 1 then
    fSmooth pvsmooth fInput, aAmpCut, aFreqCut
  elseif p4 == 2 then
    fSmooth pvsmooth fInput, aAmpCut, kFreqCut
  else
    fSmooth pvsmooth fInput, kAmpCut, aFreqCut
  endif
  rireturn
  if p4 == 0 || p4 == 3 then
    aReferenceACut = kAmpCut
  else
    aReferenceACut = aAmpCut
  endif
  if p4 == 0 || p4 == 2 then
    aReferenceFCut = kFreqCut
  else
    aReferenceFCut = aFreqCut
  endif
  aExpectedAmp, aExpectedFreq SmoothReference aInput, aReferenceACut, aReferenceFCut, aReset, p8
  kActualAmp, kActualFreq pvsbin fSmooth, p8
  ; Each block's first result depends on all preceding active samples.
  if kCycle > 1 || p5 == 0 then
    kExpectedAmp vaget 0, aExpectedAmp
    kExpectedFreq vaget 0, aExpectedFreq
    if abs(kActualAmp - kExpectedAmp) > .00001 * (1 + abs(kExpectedAmp)) || abs(kActualFreq - kExpectedFreq) > .00001 * (1 + abs(kExpectedFreq)) then
      printks "pvsmooth mode %g bin %g cycle %g: amp %g/%g, freq %g/%g\\n", 0, p4, p8, kCycle, kActualAmp, kExpectedAmp, kActualFreq, kExpectedFreq
      exitnowk -1
    endif
  endif
  ; Synthesis reads the whole frame, so inactive amplitudes must be zero.
  aRendered pvsynth fSmooth
  if kCycle == p7 && p6 < ksmps then
    kIndex = p6
    while kIndex < ksmps do
      kTail vaget kIndex, aRendered
      if kTail != 0 then
        printks "pvsmooth left output after the note ended\\n", 0
        exitnowk -1
      endif
      kIndex += 1
    od
  endif
  if kCycle == p7 then
    gkChecks += 1
  endif
endin

instr 2
  ; Ordinary frame-based smoothing keeps one update per incoming frame.
  kInput[] init 130
  kOutput[] init 130
  kHistory[] init 130
  kPrevious init 0
  kFrames init 0
  kCycle init 0
  kCycle += 1
  aInput oscili .5, 1024
  fInput pvsanal aInput, 128, 64, 128, 1
  fSmooth pvsmooth fInput, .1, .3
  kInputFrame pvs2tab kInput, fInput
  kOutputFrame pvs2tab kOutput, fSmooth
  if kInputFrame > kPrevious then
    if kOutputFrame != kInputFrame then
      printks "pvsmooth output frame count mismatch\n", 0
      exitnowk -1
    endif
    kIndex = 0
    while kIndex < 130 do
      kCut = (kIndex % 2 == 0 ? .1 : .3)
      kCosth = 2 - cos($M_PI * kCut)
      kB = kCosth - sqrt(kCosth * kCosth - 1)
      kExpected = (1 - kB) * kInput[kIndex] + kB * kHistory[kIndex]
      if abs(kOutput[kIndex] - kExpected) > .00001 * (1 + abs(kExpected)) then
        printks "pvsmooth frame %g element %g: got %g, expected %g\n", 0, kInputFrame, kIndex, kOutput[kIndex], kExpected
        exitnowk -1
      endif
      kHistory[kIndex] = kExpected
      kIndex += 1
    od
    kPrevious = kInputFrame
    kFrames += 1
  endif
  if kCycle == 32 && kFrames > 2 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 12 then
    prints "not all pvsmooth history checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Mode, first sample, final sample, block count, bin, reinit block.
i 1 0 .03125 0 0 32 8 0 0
i 1 0 .03125 1 0 32 8 24 0
i 1 0 .03125 2 0 32 8 24 0
i 1 0 .03125 3 0 32 8 0 0
i 1 0 .03125 0 0 32 8 24 5
i 1 0 .03125 1 0 32 8 24 5
; Start at sample 5 and finish at sample 112.
i 1 .0006103515625 .0130615234375 0 5 16 4 0 0
i 1 .0006103515625 .0130615234375 1 5 16 4 24 0
i 1 .0006103515625 .0130615234375 2 5 16 4 24 0
i 1 .0006103515625 .0130615234375 3 5 16 4 24 0
; Start and finish within one block.
i 1 .0006103515625 .0013427734375 0 5 16 1 24 0
i 2 0 .125
i 99 .14 .01
e
</CsScore>
</CsoundSynthesizer>
