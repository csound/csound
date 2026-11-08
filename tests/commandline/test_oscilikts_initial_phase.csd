<CsTest>
description = "oscilikts keeps its initial phase and frequency through the first block"

[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 4
nchnls = 1
0dbfs = 1

instr 1
  ; A linear ramp makes each output sample equal to its oscillator phase.
  ; Run with both power-of-two and non-power-of-two table lengths.
  iTable ftgen 0, 0, -p4, -7, 0, p4, 1
  aSync = 0
  aFrequency = 64
  aControl oscilikts 1, 64, iTable, aSync, .25
  aAudio oscilikts 1, aFrequency, iTable, aSync, .25

  kBlock init 0
  kIndex = 0
  while kIndex < ksmps do
    kExpected = frac(.25 + (kBlock*ksmps + kIndex)/16)
    kControl vaget kIndex, aControl
    kAudio vaget kIndex, aAudio
    if abs(kControl-kExpected) > .000001 || abs(kAudio-kExpected) > .000001 then
      printks "oscilikts table %g sample %g: control %g, audio %g, expected %g\n", 0, p4, kBlock*ksmps+kIndex, kControl, kAudio, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kBlock += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 8
i 1 .0625 .03125 9
</CsScore>
</CsoundSynthesizer>
