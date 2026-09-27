<CsTest>
description = "follow2 follows a lower nonzero level after changing its times"
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

instr SlowRelease
  kBlock timeinstk
  ; First settle at 1, then change both times and lower the input to -0.5.
  ; follow2 tracks absolute amplitude, so the new target is 0.5.
  kInput = (kBlock <= .1*kr ? 1 : -.5)
  kTime = (kBlock <= .1*kr ? .001 : 30)
  aInput = kInput
  aEnvelope follow2 aInput, kTime, kTime
  kLast vaget ksmps-1, aEnvelope
  kReleaseBlocks = kBlock - .1*kr

  if kReleaseBlocks == 30*kr || kReleaseBlocks == 60*kr then
    kSeconds = kReleaseBlocks/kr
    kExpected = .5 + .5*exp(-log(1000)*kSeconds/30)
    if !(abs(kLast-kExpected) < .000001) then
      printks "Release after %g seconds: expected %.9f, got %.9f\n", 0, kSeconds, kExpected, kLast
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "SlowRelease" 0 61
e
</CsScore>
</CsoundSynthesizer>
