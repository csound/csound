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

opcode CheckBank, 0, 0
  oversample 2
  kParams[] fillarray 1000, 100
  aImpulse mpulse 1, 0
  aBank resonbnk aImpulse, kParams, 0, 20000, 1
  kFirst init 1
  if kFirst == 1 then
    ; For an unscaled impulse, y1=c2 and y2=c2*c2-c3.
    kY1 vaget 1, aBank
    kY2 vaget 2, aBank
    kExpected = exp(-2*$M_PI*100/sr)
    if !(abs(kY1*kY1-kY2-kExpected) < 0.00001) then
      printks "resonbnk bandwidth does not match the local sample rate\n", 0
      exitnowk(-1)
    endif
    kFirst = 0
  endif
endop

instr 1
  CheckBank
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
