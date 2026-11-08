<CsTest>
description = "looping oscillators wrap large steps and changing endpoints"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giRamp ftgen 1, 0, 8, -2, 0, 1, 2, 3, 4, 5, 6, 7
giOne ftgen 2, 0, -1, -2, 1

instr 1
  kEnd init p6
  kExpected init p7
  kCycle timeinstk
  if p8 > 0 && kCycle == 4 then
    kEnd = p8
  endif
  ; The first pass may play the part before the loop start.
  if kExpected >= kEnd || kExpected < 0 then
    kExpected wrap kExpected, p5, kEnd
  endif
  aAmp = 1
  aLinear lposcil 1, p4, p5, kEnd, giRamp, p7
  aCubic lposcil3 1, p4, p5, kEnd, giRamp, p7
  aAudio lposcila aAmp, p4, p5, kEnd, giRamp, p7
  kLinear downsamp aLinear
  kCubic downsamp aCubic
  kAudio downsamp aAudio
  if !(abs(kLinear-kExpected) < .00001) || !(abs(kCubic-kExpected) < .00001) || !(abs(kAudio-kExpected) < .00001) then
    printks "FAIL: loop position\n", 0
    exitnowk -1
  endif
  kExpected += p4
  if kExpected >= kEnd || (p4 < 0 && kExpected < p5) then
    kExpected wrap kExpected, p5, kEnd
  endif
endin

instr 2
  aAmp = 1
  aLinear lposcil 1, 17, 0, 0, giOne
  aCubic lposcil3 1, -17, 0, 0, giOne
  aAudio lposcila aAmp, 17, 0, 0, giOne
  kLinear downsamp aLinear
  kCubic downsamp aCubic
  kAudio downsamp aAudio
  if kLinear != 1 || kCubic != 1 || kAudio != 1 then
    printks "FAIL: single-sample loop\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
; Steps larger than the table in both directions, including fractional positions.
i 1 0 .002 17 2 6 2.5 0
i 1 .01 .002 -17 2 6 2.5 0
; Keep the intro before the loop, and wrap a phase beyond a changed endpoint.
i 1 .02 .002 1 2 6 0 0
i 1 .03 .002 0 2 7 5.5 4
i 2 .04 .002
e
</CsScore>
</CsoundSynthesizer>
