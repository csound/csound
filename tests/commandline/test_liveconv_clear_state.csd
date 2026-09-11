<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iIR ftgen 0, 0, -(p4*p5), -2, 1
  kCycle init 0
  kProcessed init 0
  kLastClear init 0
  kCycle += 1
  kUpdate = (kCycle == 1 ? 1 : 0)
  kClear = (kCycle >= p6 && kCycle < p6+p8 ? p7 : 0)
  if kClear != 0 then
    kLastClear = kProcessed
  endif
  aPhase phasor 1
  aInput = aPhase*sr+1
  aOutput liveconv aInput, iIR, p4, kUpdate, kClear
  kOffset offsetsmps
  kEarly earlysmps
  kIndex = 0
  while kIndex < ksmps do
    kTime = kProcessed+kIndex-kOffset
    kExpected = 0
    if kIndex >= kOffset && kIndex < ksmps-kEarly && kTime >= kLastClear+p4 then
      kExpected = kTime-p4+1
    endif
    kActual vaget kIndex, aOutput
    if !(abs(kActual-kExpected) <= .001) then
      printks "liveconv part %g partitions %g clear %g cycle %g sample %g: got %g, expected %g\n", 0, p4, p5, p7, kCycle, kIndex, kActual, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kProcessed += ksmps-kOffset-kEarly
  if kProcessed == int(p3*sr) then
    gkChecks += 1
  endif
endin

; Update values other than exactly +1 or -1 have no effect.
instr 2
  iIR ftgen 0, 0, 128, -2, 1
  kCycle init 0
  kUpdate = (kCycle == 0 && p5 != 0 ? 1 : p4)
  aInput = 1
  aOutput liveconv aInput, iIR, 32, kUpdate, 0
  kExpected = (p5 != 0 && kCycle > 0 ? 1 : 0)
  kIndex = 0
  while kIndex < ksmps do
    kActual vaget kIndex, aOutput
    if !(abs(kActual-kExpected) <= .00001) then
      printks "liveconv acted on update value %g\n", 0, p4
      exitnowk -1
    endif
    kIndex += 1
  od
  kCycle += 1
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 3
  iIR ftgen 0, 0, -96, -2, 1
  kCycle init 0
  kCycle += 1
  kUpdate = (kCycle == 1 || kCycle == 7 ? 1 : (kCycle == 4 ? -1 : 0))
  aInput = 1
  aOutput liveconv aInput, iIR, 32, kUpdate, 0
  kExpected = (kCycle > 1 && kCycle <= 4 || kCycle > 7 ? 1 : 0)
  kIndex = 0
  while kIndex < ksmps do
    kActual vaget kIndex, aOutput
    if !(abs(kActual-kExpected) <= .00001) then
      printks "liveconv load/unload cycle %g: got %g, expected %g\n", 0, kCycle, kActual, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 20 then
    prints "liveconv checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Partition size, partition count, first clear cycle, clear value, clear cycles.
i 1 0 .0625 32 3 5 1 1
i 1 0 .0625 32 3 6 1 1
i 1 0 .0625 32 3 7 1 1
i 1 0 .0625 32 3 5 .25 1
i 1 0 .0625 32 3 5 -.25 1
i 1 0 .0625 8 3 5 1 1
i 1 0 .0625 4 3 5 1 1
i 1 0 .0625 64 3 4 1 1
i 1 0 .0625 128 3 4 1 1
i 1 0 .0625 32 1 5 1 2
i 1 0 .0625 32 3 5 .25 2
i 1 .0006103515625 .0130615234375 8 3 2 1 1
i 1 .0006103515625 .0013427734375 8 3 0 0 0
i 1 .125 .0625 32 3 5 1 1
i 2 .25 .0625 .6 0
i 2 .25 .0625 -.6 1
i 2 .25 .0625 1e20 0
i 2 .25 .0625 -1e20 1
i 3 .375 .0625
i 3 .5 .0625
i 99 .625 .01
e
</CsScore>
</CsoundSynthesizer>
