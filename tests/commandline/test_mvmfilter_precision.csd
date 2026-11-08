<CsTest>
description = "mvmfilter preserves long decays and tuning in float builds"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckImpulse
  iFrequency = p4
  iDecay = p5
  aImpulse mpulse 1, 0
  kFrequency = iFrequency
  kDecay = iDecay
  aFrequency = kFrequency
  aDecay = kDecay
  aKK mvmfilter aImpulse, kFrequency, kDecay
  aAK mvmfilter aImpulse, aFrequency, kDecay
  aKA mvmfilter aImpulse, kFrequency, aDecay
  aAA mvmfilter aImpulse, aFrequency, aDecay

  kBlock init 0
  if kBlock == kr then
    ; All frequencies below are whole numbers of Hz. After one second,
    ; the phase has completed whole cycles. Check one sample later so
    ; phase error is visible as well as decay error.
    iExpected = exp(-(sr+1)/(sr*iDecay))*cos(2*$M_PI*iFrequency/sr)
    kKK vaget 1, aKK
    kAK vaget 1, aAK
    kKA vaget 1, aKA
    kAA vaget 1, aAA
    if !(abs(kKK-iExpected) < .00001 && abs(kAK-iExpected) < .00001 && abs(kKA-iExpected) < .00001 && abs(kAA-iExpected) < .00001) then
      printks "mvmfilter frequency=%g decay=%g: expected %.12g, kk %.12g, ak %.12g, ka %.12g, aa %.12g\n", 0, iFrequency, iDecay, iExpected, kKK, kAK, kKA, kAA
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kBlock += 1
endin

instr CheckCompletion
  if i(gkChecks) != 3 then
    prints "The long-decay and both tuning checks must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Zero frequency isolates the long decay from the rotation.
i "CheckImpulse" 0 1.01 0 1024
; Higher frequencies expose phase error accumulated over many samples.
i "CheckImpulse" 0 1.01 11025 2
i "CheckImpulse" 0 1.01 23000 2
i "CheckCompletion" 1.02 .001
e
</CsScore>
</CsoundSynthesizer>
