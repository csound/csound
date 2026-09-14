<CsTest>
description = "test weighted spectrum addition and its default"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  kSignal1 oscili .1, 220
  kSignal2 oscili .075, 110
  wFirst spectrum kSignal1, .01, 3, 12, 10, 0, 0, 0
  wSecond spectrum kSignal2, .01, 3, 12, 10, 0, 0, 0
  wAdded specaddm wFirst, wSecond, p4
  wDefault specaddm wFirst, wSecond
  kFirst specsum wFirst
  kSecond specsum wSecond
  kAdded specsum wAdded
  kDefault specsum wDefault
  kExpected = kFirst + p4*kSecond
  kCount timeinstk

  if abs(kAdded-kExpected) > .00001 then
    printks "specaddm multiplier %g: got %g, expected %g\n", 0, p4, kAdded, kExpected
    exitnowk 1
  endif
  if abs(kDefault-kFirst) > .00001 then
    printks "specaddm omitted multiplier must default to zero\n", 0
    exitnowk 1
  endif
  if kCount == 1000 && (kFirst <= .001 || kSecond <= .001) then
    printks "specaddm inputs did not produce nonzero spectra\n", 0
    exitnowk 1
  endif
endin
</CsInstruments>
<CsScore>
; Reuse the instrument with different multipliers to check initialization.
i 1 0   .5  1
i 1 .6  .5  0
i 1 1.2 .5  2
i 1 1.8 .5 -.5
i 1 2.4 .5  .5
e
</CsScore>
</CsoundSynthesizer>
