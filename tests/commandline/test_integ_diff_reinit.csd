<CsTest>
description = "integ and diff reset or preserve state on reinit, including in-place use"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckState
  kStep init 0
  kInput = kStep + 1
  aInput = kInput
  kInPlace = kInput
  aInPlace = aInput
  ; Reinitialize once, after processing the inputs 1, 2, and 3.
  if kStep == 3 then
    reinit FILTERS
  endif
FILTERS:
  kSum integ kInput, p4
  aSum integ aInput, p4
  kDifference diff kInput, p4
  aDifference diff aInput, p4
  ; Integrating then differentiating exact integer sums restores the input.
  kInPlace integ kInPlace, p4
  kInPlace diff kInPlace, p4
  aInPlace integ aInPlace, p4
  aInPlace diff aInPlace, p4
  rireturn

  kExpectedSum = kInput*(kInput+1)/2
  kExpectedDifference = 1
  if p4 == 0 && kStep >= 3 then
    kExpectedSum -= 6
    if kStep == 3 then
      kExpectedDifference = 4
    endif
  endif
  kAudioSum downsamp aSum
  kAudioDifference downsamp aDifference
  kAudioInPlace downsamp aInPlace
  if !(kSum == kExpectedSum && kAudioSum == kExpectedSum) then
    printks "skip=%g step=%g: expected sum %g, got %g/%g\n", 0, p4, kStep, kExpectedSum, kSum, kAudioSum
    exitnowk -1
  endif
  if !(kDifference == kExpectedDifference && kAudioDifference == kExpectedDifference) then
    printks "skip=%g step=%g: expected difference %g, got %g/%g\n", 0, p4, kStep, kExpectedDifference, kDifference, kAudioDifference
    exitnowk -1
  endif
  if !(kInPlace == kInput && kAudioInPlace == kInput) then
    printks "skip=%g step=%g: expected restored input %g, got %g/%g\n", 0, p4, kStep, kInput, kInPlace, kAudioInPlace
    exitnowk -1
  endif
  kStep += 1
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 16 then
    prints "Both reinit cases must complete eight steps\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; p4=0 resets the state; p4=1 preserves it.
i "CheckState" 0 .25 0
i "CheckState" 0 .25 1
i "CheckCompletion" .25 .03125
e
</CsScore>
</CsoundSynthesizer>
