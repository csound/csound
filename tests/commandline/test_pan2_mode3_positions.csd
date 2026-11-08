<CsTest>
description = "pan2 mode 3 follows the documented left-to-right pan range"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 2
0dbfs = 1
gkChecks init 0
gkFailures init 0

instr CheckPosition
  iMode = p4
  iPan = p5
  iExpectedLeft = p6
  iExpectedRight = p7
  aInput = 1
  kPan = iPan
  aPan = iPan
  aControlLeft, aControlRight pan2 aInput, kPan, iMode
  aAudioLeft, aAudioRight pan2 aInput, aPan, iMode
  aControlPair[] pan2 aInput, kPan, iMode
  aAudioPair[] pan2 aInput, aPan, iMode
  kLeft downsamp aControlLeft
  kRight downsamp aControlRight
  aError = abs(aControlLeft-iExpectedLeft) + abs(aControlRight-iExpectedRight)
  aError += abs(aAudioLeft-iExpectedLeft) + abs(aAudioRight-iExpectedRight)
  aError += abs(aControlPair[0]-iExpectedLeft) + abs(aControlPair[1]-iExpectedRight)
  aError += abs(aAudioPair[0]-iExpectedLeft) + abs(aAudioPair[1]-iExpectedRight)
  kError downsamp aError
  if !(kError < .00001) then
    printks "pan2 mode=%g pan=%g expected=(%g,%g) actual=(%g,%g) combined error=%g\n", \
      0, iMode, iPan, iExpectedLeft, iExpectedRight, kLeft, kRight, kError
    gkFailures += 1
  endif
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 7 || i(gkFailures) != 0 then
    prints "pan2 checked %g positions, with %g failures\n", i(gkChecks), i(gkFailures)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; The manual defines pan=0 as hard left and pan=1 as hard right.
; Mode 3 is an equal-power mode, so its center gains should both be sqrt(.5).
;                              mode pan expected left expected right
i "CheckPosition" 0 [1/8192]     3   0    1             0
i "CheckPosition" 0 [1/8192]     3   .5   [.5^.5]      [.5^.5]
i "CheckPosition" 0 [1/8192]     3   1    0             1
; Quarter positions must remain symmetric across the stereo field.
i "CheckPosition" 0 [1/8192]     3   .25  .9238795325112867 .3826834323650898
i "CheckPosition" 0 [1/8192]     3   .75  .3826834323650898 .9238795325112867
; The default mode already follows the documented endpoints.
i "CheckPosition" 0 [1/8192]     0   0    1             0
i "CheckPosition" 0 [1/8192]     0   1    0             1
i "CheckResults" [2/8192] [1/8192]
e
</CsScore>
</CsoundSynthesizer>
