<CsTest>
description = "lineto and tlineto advance throughout small ramps"
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

instr SmallRamp
  iStart = p4
  iTarget = p5
  kBlock timeinstk
  ; Initialize at the starting value, then request a 30-second ramp.
  kInput = (kBlock == 1 ? iStart : iTarget)
  kTrigger = (kBlock == 2 ? 1 : 0)
  kLine lineto kInput, 30
  kTriggered tlineto kInput, 30, kTrigger

  ; Reaching the endpoint alone would miss a ramp that stayed flat,
  ; so also check halfway through the rise or fall.
  if kBlock == 15*kr + 2 || kBlock == 30*kr + 2 then
    kSeconds = (kBlock-2)/kr
    kExpected = iStart + (iTarget-iStart)*kSeconds/30
    if !(abs(kLine-kExpected) < .000001 && abs(kTriggered-kExpected) < .000001) then
      printks "Ramp from %g to %g after %g seconds: expected %.9f, lineto %.9f, tlineto %.9f\n", 0, iStart, iTarget, kSeconds, kExpected, kLine, kTriggered
      exitnowk -1
    endif
  endif
  if kBlock == 30*kr + 3 && (kLine != iTarget || kTriggered != iTarget) then
    printks "lineto and tlineto must hold the exact target after the ramp\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
; Both increments are smaller than one float step near 1.
i "SmallRamp" 0 31 1 1.001
i "SmallRamp" 0 31 1 .999
e
</CsScore>
</CsoundSynthesizer>
