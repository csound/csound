<CsTest>
description = "integ retains small increments within and between audio blocks"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckSum
  kStep init 0
  ; Start with 1 (or -1), then keep adding a small value of the same sign.
  kInput = p4 * (kStep == 0 ? 1 : .00000001)
  aInput = kInput
  kSum integ kInput
  aSum integ aInput
  ; Also allow the input and output to share the same variable.
  kInPlace = kInput
  kInPlace integ kInPlace
  aInPlace = aInput
  aInPlace integ aInPlace

  if kStep == 1024 then
    kExpected = p4 * (1 + kStep*.00000001)
    ; Each audio block contains ksmps copies of the control input.
    kExpectedAudio = ksmps * kExpected
    kAudio vaget ksmps-1, aSum
    kAudioInPlace vaget ksmps-1, aInPlace
    if !(abs(kSum-kExpected) < .000001 && abs(kInPlace-kExpected) < .000001) then
      printks "Control sum: expected %.9f, got %.9f (in-place %.9f)\n", 0, kExpected, kSum, kInPlace
      exitnowk -1
    endif
    if !(abs(kAudio-kExpectedAudio) < .000001 && abs(kAudioInPlace-kExpectedAudio) < .000001) then
      printks "Audio sum: expected %.9f, got %.9f (in-place %.9f)\n", 0, kExpectedAudio, kAudio, kAudioInPlace
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 2 then
    prints "Both sums must reach the check point\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckSum" 0 5 1
i "CheckSum" 0 5 -1
i "CheckCompletion" 5 .01
e
</CsScore>
</CsoundSynthesizer>
