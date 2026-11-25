<CsTest>
description = "swtanal honours a mid-block note start with a local setksmps"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32000
ksmps = 32
nchnls = 1
0dbfs = 1
gkEarlyChecks init 0
gkOnsetChecks init 0
gkFreshChecks init 0

instr 1
  setksmps 1
  aInput = p4
  restartWave@global:swt swtanal aInput, 8, 1, 1, 2, 3, 0, 1, 2
endin

instr 2
  aOutput swtsynth restartWave
  kCycle init 0
  kCycle += 1
  if kCycle == 4 then
    kEarly vaget 8, aOutput
    if abs(kEarly) > 0.000001 then
      printks "swt local ksmps restart played early: sample=%d value=%g\n", 0, 104, kEarly
      exitnowk(-1)
    endif
    kOnset vaget 24, aOutput
    if abs(kOnset - 0.5) > 0.0005 then
      printks "swt local ksmps restart missed onset: sample=%d value=%g\n", 0, 120, kOnset
      exitnowk(-1)
    endif
    gkEarlyChecks += 1
    gkOnsetChecks += 1
  endif
endin

instr 3
  aFresh swtsynth restartWave
  kCycle init 0
  kCycle += 1
  if kCycle == 1 then
    kOnset vaget 24, aFresh
    if abs(kOnset - 0.5) > 0.0005 then
      printks "swt local ksmps fresh synth reference missed onset: value=%g\n", 0, kOnset
      exitnowk(-1)
    endif
    gkFreshChecks += 1
  endif
endin

instr 4
  if i(gkEarlyChecks) != 1 || i(gkOnsetChecks) != 1 || i(gkFreshChecks) != 1 then
    prints "swt local ksmps restart checks did not complete: %d %d %d\n", i(gkEarlyChecks), i(gkOnsetChecks), i(gkFreshChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.0035 0
i 1 0.0035 0.01 0.5
i 2 0 0.02
i 3 0.0035 0.01
i 4 0.021 0.01
e
</CsScore>
</CsoundSynthesizer>
