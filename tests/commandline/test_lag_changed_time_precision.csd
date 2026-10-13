<CsTest>
description = "lag and lagud retain precision after changing their lag times"
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

instr ChangedTime
  kBlock timeinstk
  ; Keep the input at its initial level until the new coefficients settle.
  ; Then a small step must still reach its target, rather than stay at 1.
  kTime = (kBlock == 1 ? .01 : 30)
  kInput = (kBlock <= 2 ? 1 : 1.001)
  aInput = kInput
  kLag lag kInput, kTime, 1
  kLagUD lagud kInput, kTime, kTime, 1
  aLag lag aInput, kTime, 1
  aLagUD lagud aInput, kTime, kTime, 1
  kAudioLag vaget ksmps-1, aLag
  kAudioLagUD vaget ksmps-1, aLagUD

  if kBlock == 30*kr + 2 then
    kExpected = 1 + (kInput-1)*.999
    if !(abs(kLag-kExpected) < .000001 && abs(kLagUD-kExpected) < .000001 && \
         abs(kAudioLag-kExpected) < .000001 && abs(kAudioLagUD-kExpected) < .000001) then
      printks "Changed lag time: expected %.9f, control %.9f/%.9f, audio %.9f/%.9f\n", 0, kExpected, kLag, kLagUD, kAudioLag, kAudioLagUD
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "ChangedTime" 0 31
e
</CsScore>
</CsoundSynthesizer>
