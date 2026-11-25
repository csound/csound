<CsTest>
description = "swtsynth and swtset replay a changed signal immediately when a global source restarts with the same layout"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0
gkReferenceChecks init 0
gkActualPrev[] init 32
gkSetActualPrev[] init 32
gaRefOutput init 0
gaRefSetOutput init 0

instr 1
  aInput = 0.5 * oscili(1, p4)
  restartWave@global:swt swtanal aInput, 32, 32, 1, 2, 4, 0, 0, 1
endin

instr 5
  aInput = 0.5 * oscili(1, p4)
  refRestartWave@global:swt swtanal aInput, 32, 32, 1, 2, 4, 0, 0, 1
endin

instr 6
  kRefCoeffs:k[] swtget refRestartWave, -1
  refSet@global:swt swtset refRestartWave, kRefCoeffs, -1
endin

instr 7
  aRef swtsynth refRestartWave
  aRefSet swtsynth refSet
  gaRefOutput = aRef
  gaRefSetOutput = aRefSet
endin

instr 2
  kCoeffs:k[] swtget restartWave, -1
  restartSet@global:swt swtset restartWave, kCoeffs, -1
endin

instr 3
  aOutput swtsynth restartWave
  aSetOutput swtsynth restartSet
  kCycle init 0
  kCycle += 1
  if kCycle >= 185 && kCycle <= 200 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      kSetActual vaget kIndex, aSetOutput
      if kCycle > 185 then
        kWanted vaget kIndex, gaRefOutput
        kWantedSet vaget kIndex, gaRefSetOutput
        kActualPrev = gkActualPrev[kIndex]
        kSetActualPrev = gkSetActualPrev[kIndex]
        if abs(kActualPrev - kWanted) > 0.0005 then
          printks "swt restart synth failed: cycle=%d sample=%d got=%g expected=%g\n", 0, kCycle, kIndex, kActualPrev, kWanted
          exitnowk(-1)
        endif
        if abs(kSetActualPrev - kWantedSet) > 0.0005 then
          printks "swt restart swtset failed: cycle=%d sample=%d got=%g expected=%g\n", 0, kCycle, kIndex, kSetActualPrev, kWantedSet
          exitnowk(-1)
        endif
        if abs(kWanted) > 0.0001 then
          gkReferenceChecks += 1
        endif
        gkChecks += 1
      endif
      gkActualPrev[kIndex] = kActual
      gkSetActualPrev[kIndex] = kSetActual
      kIndex += 1
    od
  endif
endin

instr 4
  if i(gkChecks) != 480 || i(gkReferenceChecks) < 1 then
    prints "swt restart checks did not complete: %d %d\n", i(gkChecks), i(gkReferenceChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.09 997
i 1 0.11 0.1 1499
i 5 0.11 0.2 1499
i 6 0.11 0.2
i 7 0.11 0.2
i 2 0 0.25
i 3 0 0.25
i 4 0.26 0.01
e
</CsScore>
</CsoundSynthesizer>
