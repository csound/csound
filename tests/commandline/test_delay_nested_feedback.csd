<CsTest>
description = "Nested delay lines in one instrument keep their writer order and feedback"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkCompleted init 0

instr NestedFeedback
  ; Different lengths make the buffers wrap at different points in a block.
  iFirstSamples = 37
  iSecondSamples = 53
  iFirstImpulse = p4
  aImpulse mpulse 1, 0

  ; Both readers precede both writers. The first writer belongs to the
  ; first reader, even though the second reader appears closer to it.
  aFirst delayr iFirstSamples / sr
  aSecond delayr iSecondSamples / sr
  delayw iFirstImpulse*aImpulse - aFirst
  delayw aImpulse - aSecond

  ; Each loop returns its impulse after one delay, then repeats it with
  ; alternating signs. Every sample between those echoes must be zero.
  kSample init 0
  kFrame = 0
  while kFrame < ksmps do
    kExpectedFirst = 0
    if kSample > 0 && kSample % iFirstSamples == 0 then
      kEcho = kSample / iFirstSamples
      kExpectedFirst = (kEcho % 2 == 1 ? iFirstImpulse : -iFirstImpulse)
    endif
    kExpectedSecond = 0
    if kSample > 0 && kSample % iSecondSamples == 0 then
      kEcho = kSample / iSecondSamples
      kExpectedSecond = (kEcho % 2 == 1 ? 1 : -1)
    endif

    kFirst vaget kFrame, aFirst
    kSecond vaget kFrame, aSecond
    if kFirst != kExpectedFirst || kSecond != kExpectedSecond then
      printks "Nested delays at sample %g returned %g, %g instead of %g, %g\n", 0, kSample, kFirst, kSecond, kExpectedFirst, kExpectedSecond
      exitnowk -1
    endif
    kSample += 1
    kFrame += 1
  od
  if kSample == 512 then
    gkCompleted += 1
  endif
endin

instr CheckCompletion
  if i(gkCompleted) != 2 then
    prints "Both nested feedback cases must run for 512 samples\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; First, leave the first loop silent and feed the impulse into the second.
i "NestedFeedback" 0 .0625 0
; Then excite both loops, with a smaller impulse in the first.
i "NestedFeedback" .125 .0625 .5
i "CheckCompletion" .25 0
e
</CsScore>
</CsoundSynthesizer>
