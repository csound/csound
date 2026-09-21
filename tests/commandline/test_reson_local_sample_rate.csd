<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 4
nchnls = 1
0dbfs = 1

opcode CheckResonators, 0, 0
  oversample 2
  aImpulse mpulse 1, 0
  aReson reson aImpulse, 1000, 100, 0
  aResonx resonx aImpulse, 1000, 100, 1, 0
  aAreson areson aImpulse, 1000, 100, 0
  kFirst init 1
  if kFirst == 1 then
    ; Recover the bandwidth coefficient from the first impulse samples.
    ; reson/resonx: y1=c2, y2=c2^2-c3; areson: y1=-c2, y2=-c2^2+c3.
    kR1 vaget 1, aReson
    kR2 vaget 2, aReson
    kX1 vaget 1, aResonx
    kX2 vaget 2, aResonx
    kA1 vaget 1, aAreson
    kA2 vaget 2, aAreson
    kExpected = exp(-2*$M_PI*100/sr)
    if abs(kR1*kR1-kR2-kExpected) > 0.00001 || \
       abs(kX1*kX1-kX2-kExpected) > 0.00001 || \
       abs(kA1*kA1+kA2-kExpected) > 0.00001 then
      printks "resonator bandwidth does not match the local sample rate\n", 0
      exitnowk(-1)
    endif
    kFirst = 0
  endif
endop

instr 1
  CheckResonators
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
