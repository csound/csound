<CsTest>
description = "follow2 reaches the expected level during a slow attack"
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

instr SlowAttack
  aInput = 1
  aEnvelope follow2 aInput, 30, 30
  kLast vaget ksmps-1, aEnvelope
  kBlock timeinstk

  ; A 60 dB attack reduces the distance to 1 by a factor of 1000.
  ; Check again after two attack times to catch a stalled envelope.
  if kBlock == 30*kr || kBlock == 60*kr then
    kSeconds = kBlock/kr
    kExpected = 1 - exp(-log(1000)*kSeconds/30)
    if !(abs(kLast-kExpected) < .000001) then
      printks "Attack after %g seconds: expected %.9f, got %.9f\n", 0, kSeconds, kExpected, kLast
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "SlowAttack" 0 61
e
</CsScore>
</CsoundSynthesizer>
