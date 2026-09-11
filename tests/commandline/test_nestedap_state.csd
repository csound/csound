<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; A reset must match a fresh filter, even when its mode or inner delays
; change without changing the total buffer size. A skipped reset must
; match the original filter running without interruption.
instr 1
 setksmps 1
 kCycle init 0
 kCycle += 1
 kInput = (kCycle < 19 ? 1 : (kCycle < 35 ? 0 : (kCycle % 7 - 3) * .1))
 aInput = kInput
 if kCycle == 19 then
  reinit FILTER
 endif
FILTER:
 iChanged = (i(kCycle) >= 19 ? 1 : 0)
 iMode = (iChanged == 1 ? p5 : p4)
 iDelay2 = (iChanged == 1 ? 6 : 4) / sr
 iDelay3 = (iChanged == 1 ? 2 : 3) / sr
 aActual nestedap aInput, iMode, 1, 16/sr, .2, iDelay2, .3, iDelay3, .4, p6
 rireturn
 aContinuous nestedap aInput, p4, 1, 16/sr, .2, 4/sr, .3, 3/sr, .4, 1
 if kCycle >= 19 then
  aFresh nestedap aInput, p5, 1, 16/sr, .2, 6/sr, .3, 2/sr, .4
  kActual downsamp aActual
  if p6 == 0 then
   kExpected downsamp aFresh
  else
   kExpected downsamp aContinuous
  endif
  if !(abs(kActual - kExpected) < .00001) then
   printks "nestedap mode=%g->%g skip=%g cycle=%g: got=%g expected=%g\n", 0, p4, p5, p6, kCycle, kActual, kExpected
   exitnowk(-1)
  endif
 endif
 if kCycle == 64 then
  gkChecks += 1
 endif
endin

; A pure three-sample delay over a partial block, also with reused input.
instr 2
 aInput = 1
 if p4 == 0 then
  aResult nestedap aInput, 1, 1, 3/sr, 0
 else
  aInput nestedap aInput, 1, 1, 3/sr, 0
  aResult = aInput
 endif
 kIndex = 0
 while kIndex < 16 do
  kActual vaget kIndex, aResult
  kExpected = (kIndex >= 6 && kIndex < 11 ? 1 : 0)
  if kActual != kExpected then
   printks "nestedap partial sample=%g: got=%g expected=%g\n", 0, kIndex, kActual, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 gkChecks += 1
endin
instr 99
 if i(gkChecks) != 20 then
  prints "nestedap checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125 1 1 0
i 1 0 .0078125 1 2 0
i 1 0 .0078125 1 3 0
i 1 0 .0078125 2 1 0
i 1 0 .0078125 2 2 0
i 1 0 .0078125 2 3 0
i 1 0 .0078125 3 1 0
i 1 0 .0078125 3 2 0
i 1 0 .0078125 3 3 0
i 1 0 .0078125 1 1 1
i 1 0 .0078125 1 2 1
i 1 0 .0078125 1 3 1
i 1 0 .0078125 2 1 1
i 1 0 .0078125 2 2 1
i 1 0 .0078125 2 3 1
i 1 0 .0078125 3 1 1
i 1 0 .0078125 3 2 1
i 1 0 .0078125 3 3 1
i 2 .0159912109375 .0009765625 0
i 2 .0198974609375 .0009765625 1
i 99 .03125 .001953125
e
</CsScore>
</CsoundSynthesizer>
