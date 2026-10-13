<CsTest>
description = "lag and lagud keep moving during slow rises and falls"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

instr SlowLag
  iStart = p4
  iTarget = p5
  iRiseTime = (iTarget > iStart ? 30 : .01)
  iFallTime = (iTarget < iStart ? 30 : .01)
  kInput = iTarget
  aInput = iTarget
  kLag lag kInput, 30, iStart
  kLagUD lagud kInput, iRiseTime, iFallTime, iStart
  aLag lag aInput, 30, iStart
  aLagUD lagud aInput, iRiseTime, iFallTime, iStart
  kAudioLag vaget ksmps-1, aLag
  kAudioLagUD vaget ksmps-1, aLagUD
  kBlock timeinstk

  ; Each 30 seconds reduces the distance to the target by a factor of 1000.
  ; The audio coefficient ramps across the first block; its small timing
  ; difference fits within the tolerance at these checkpoints.
  if kBlock == 30*kr || kBlock == 60*kr then
    kSeconds = kBlock/kr
    kExpected = iTarget + (iStart-iTarget)*exp(-log(1000)*kSeconds/30)
    if !(abs(kLag-kExpected) < .000001 && abs(kLagUD-kExpected) < .000001 && \
         abs(kAudioLag-kExpected) < .000001 && abs(kAudioLagUD-kExpected) < .000001) then
      printks "Lag from %g to %g after %g seconds: expected %.9f, control %.9f/%.9f, audio %.9f/%.9f\n", 0, iStart, iTarget, kSeconds, kExpected, kLag, kLagUD, kAudioLag, kAudioLagUD
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
; Check rising from zero and falling toward a nonzero target.
i "SlowLag" 0 61 0 1
i "SlowLag" 0 61 1 .5
e
</CsScore>
</CsoundSynthesizer>
