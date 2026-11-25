<CsTest>
description = "swtsynth and swtset follow a same-layout global switch between two analyzers with distinct stream identities"

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
gkReferenceChecks init 0

instr 1
  aInput = p4
  selectedWave@global:swt swtanal aInput, 32, 32, 1, 2, 4, 0, 0, 1
endin

instr 5
  aInput = p4
  waveB@global:swt swtanal aInput, 32, 32, 1, 2, 4, 0, 0, 1
endin

instr 6
  kCycle init 0
  kCycle += 1
  if kCycle >= 4 then
    selectedWave = waveB
  endif
endin

instr 2
  kCoeffs:k[] swtget selectedWave, -1
  editedWave@global:swt swtset selectedWave, kCoeffs, -1
  aOutput swtsynth selectedWave
  aSetOutput swtsynth editedWave
  kCycle init 0
  kCycle += 1
  if kCycle >= 150 && kCycle <= 160 then
    kActual vaget 0, aOutput
    kSetActual vaget 0, aSetOutput
    if abs(kActual - 0.5) > 0.0005 then
      printks "swt analyzer switch synth failed: cycle=%d value=%g\n", 0, kCycle, kActual
      exitnowk(-1)
    endif
    if abs(kSetActual - 0.5) > 0.0005 then
      printks "swt analyzer switch swtset failed: cycle=%d value=%g\n", 0, kCycle, kSetActual
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin

instr 7
  aFresh swtsynth waveB
  kCycle init 0
  kCycle += 1
  if kCycle >= 6 && kCycle <= 10 then
    kActual vaget 0, aFresh
    if abs(kActual - 0.5) > 0.0005 then
      printks "swt analyzer switch fresh synth failed: cycle=%d value=%g\n", 0, kCycle, kActual
      exitnowk(-1)
    endif
    gkReferenceChecks += 1
  endif
endin

instr 4
  if i(gkChecks) != 11 || i(gkReferenceChecks) != 5 then
    prints "swt analyzer switch checks did not complete: %d %d\n", i(gkChecks), i(gkReferenceChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1 0.25
i 5 0.1 0.25 0.5
i 6 0.1 0.2
i 2 0 0.35
i 7 0.1 0.25
i 4 0.36 0.01
e
</CsScore>
</CsoundSynthesizer>
