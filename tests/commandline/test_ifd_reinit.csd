<CsTest>
description = "IFD opcodes reset frames and windows when reinitialized at a smaller FFT size"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
giSource ftgen 0, 0, 256, 10, 1, .3
gkChecks init 0

instr CheckReset
 kSize init 256
 kWindow init 0
 kCycle init 0
 kCycle += 1
 if kCycle == 20 then
  kSize = 64
  kWindow = 1
  reinit ANALYZE
 endif
 aInput oscili .25, 437
ANALYZE:
 if p4 == 0 then
  fFrequency, fPhase pvsifd aInput, i(kSize), 32, i(kWindow)
  fReference, fReferencePhase pvsifd aInput, 64, 32, 1
 else
  fFrequency, fPhase tabifd -.5/sr, 1, .75, i(kSize), 32, i(kWindow), giSource
  fReference, fReferencePhase tabifd -.5/sr, 1, .75, 64, 32, 1, giSource
 endif
 kActual[] init 258
 kActualPhase[] init 258
 kReference[] init 66
 kReferencePhase[] init 66
 rireturn
 kFrame pvs2array kActual, fFrequency
 kPhaseCount pvs2array kActualPhase, fPhase
 kReferenceCount pvs2array kReference, fReference
 kReferencePhaseCount pvs2array kReferencePhase, fReferencePhase
 ; Both instances restart together. After the change, the one that reuses
 ; larger buffers must agree with the fixed-size reference on every field.
 if kCycle >= 20 then
  kIndex = 0
  while kIndex < 66 do
   kError = abs(kActual[kIndex]-kReference[kIndex]) + abs(kActualPhase[kIndex]-kReferencePhase[kIndex])
   if !(kError < 1e-5) then
    printks "IFD reinit: table=%g cycle=%g field=%g error=%g\n", 0, p4, kCycle, kIndex, kError
    exitnowk(-1)
   endif
   kIndex += 1
  od
 endif
 if kCycle == 40 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 2 then
  prints "IFD reinit checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckReset" 0 .10 0
i "CheckReset" 0 .10 1
i "CheckResults" .11 .01
e
</CsScore>
</CsoundSynthesizer>
