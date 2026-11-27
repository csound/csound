<CsTest>
description = "swtanal and swtsynth reconstruct direct and overlap-add frames from latency onward, start late synths correctly, and clear inactive output samples"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0
gkOnsetChecks init 0
gkInactiveChecks init 0
gkLateSilent init 0
gkLateAudio init 0
gkLateOlaSilent init 0
gkLateOlaAudio init 0

instr 1
  iN = p4
  iH = p5
  iWindow = p6
  iOla = p7
  aTone oscili 0.1, 997
  aInput = 0.05 + aTone
  aExpected delay aInput, iN/sr
  w:swt swtanal aInput, iN, iH, 2, 4, 4, 0, iWindow, iOla
  aOutput swtsynth w
  kCycle init 0
  kCycle += 1
  if kCycle >= 6 && kCycle <= 14 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      kWanted vaget kIndex, aExpected
      if abs(kActual - kWanted) > 0.0005 then
        printks "swt reconstruction failed: N=%d H=%d cycle=%d sample=%d got=%g expected=%g\n", 0, iN, iH, kCycle, kIndex, kActual, kWanted
        exitnowk(-1)
      endif
      kIndex += 1
    od
    gkChecks += 1
  endif
endin

instr 3
  aInput = 0.25
  aExpected delay aInput, 64/sr
  w:swt swtanal aInput, 64, 16, 1, 4, 4, 0, 1, 2
  aOutput swtsynth w
  kCycle init 0
  kCycle += 1
  if kCycle >= 3 && kCycle <= 6 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      kWanted vaget kIndex, aExpected
      if abs(kActual - kWanted) > 0.0005 then
        printks "swt overlap-add onset failed: cycle=%d sample=%d got=%g expected=%g\n", 0, kCycle, kIndex, kActual, kWanted
        exitnowk(-1)
      endif
      kIndex += 1
    od
    gkOnsetChecks += 1
  endif
endin

instr 4
  aInput = 0.25
  w:swt swtanal aInput, 32, 32, 1, 4, 4, 0, 0, 1
  aOutput swtsynth w
  kCycle init 0
  kCycle += 1
  if kCycle == 5 then
    kActual vaget 8, aOutput
    if abs(kActual) > 0.000001 then
      printks "swtsynth left an inactive sample: cycle=%d sample=8 value=%g\n", 0, kCycle, kActual
      exitnowk(-1)
    endif
    gkInactiveChecks += 1
  endif
endin

instr 5
  aInput = 0.25
  lateWave@global:swt swtanal aInput, 32, 32, 1, 4, 4, 0, 0, 1
endin

instr 6
  aOutput swtsynth lateWave
  kCycle init 0
  kCycle += 1
  if kCycle == 1 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      if abs(kActual) > 0.000001 then
        printks "late swtsynth replayed an old frame: cycle=%d sample=%d value=%g\n", 0, kCycle, kIndex, kActual
        exitnowk(-1)
      endif
      kIndex += 1
      gkLateSilent += 1
    od
  elseif kCycle == 3 then
    kActual vaget 0, aOutput
    if abs(kActual - 0.25) > 0.0005 then
      printks "late swtsynth did not consume new frames: cycle=%d value=%g\n", 0, kCycle, kActual
      exitnowk(-1)
    endif
    gkLateAudio += 1
  endif
endin

instr 7
  aInput = 0.25
  lateOlaWave@global:swt swtanal aInput, 64, 16, 1, 4, 4, 0, 1, 2
endin

instr 8
  aOutput swtsynth lateOlaWave
  kCycle init 0
  kCycle += 1
  if kCycle <= 2 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      if abs(kActual) > 0.000001 then
        printks "late overlap-add swtsynth replayed an old frame: cycle=%d sample=%d value=%g\n", 0, kCycle, kIndex, kActual
        exitnowk(-1)
      endif
      gkLateOlaSilent += 1
      kIndex += 1
    od
  elseif kCycle >= 4 && kCycle <= 6 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      if abs(kActual - 0.25) > 0.0005 then
        printks "late overlap-add swtsynth onset failed: cycle=%d sample=%d got=%g expected=0.25\n", 0, kCycle, kIndex, kActual
        exitnowk(-1)
      endif
      kIndex += 1
    od
    gkLateOlaAudio += 1
  endif
endin

instr 2
  if i(gkChecks) != 18 || i(gkOnsetChecks) != 4 || i(gkInactiveChecks) != 1 || i(gkLateSilent) != 32 || i(gkLateAudio) != 1 || i(gkLateOlaSilent) != 64 || i(gkLateOlaAudio) != 3 then
    prints "swt reconstruction checks did not complete: %d %d %d %d %d %d %d\n", i(gkChecks), i(gkOnsetChecks), i(gkInactiveChecks), i(gkLateSilent), i(gkLateAudio), i(gkLateOlaSilent), i(gkLateOlaAudio)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.2 32 32 0 1
i 1 0 0.2 64 16 1 2
i 3 0 0.2
i 4 0 0.003
i 5 0 0.2
i 6 0.05 0.1
i 7 0 0.2
i 8 0.05 0.12
i 2 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
