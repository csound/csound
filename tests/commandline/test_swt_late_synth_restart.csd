<CsTest>
description = "swtsynth keeps a late synth origin through a producer restart"

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
gkBeforeChecks init 0
gkRunningChecks init 0
gkFreshChecks init 0

instr 1
  aInput = p4
  lateWave@global:swt swtanal aInput, 32, 32, 1, 2, 3, 0, 0, 1
endin

instr 2
  aOutput swtsynth lateWave
  kCycle init 0
  kCycle += 1
  if kCycle == 25 then
    kBefore vaget 0, aOutput
    if abs(kBefore - 0.25) > 0.001 then
      printks "swt late synth lost pre-restart level: value=%g\n", 0, kBefore
      exitnowk(-1)
    endif
    gkBeforeChecks += 1
  endif
  if kCycle == 27 then
    kAfter vaget 0, aOutput
    if abs(kAfter - 0.5) > 0.001 then
      printks "swt late synth lost post-restart level: value=%g\n", 0, kAfter
      exitnowk(-1)
    endif
    gkRunningChecks += 1
  endif
endin

instr 3
  aFresh swtsynth lateWave
  kCycle init 0
  kCycle += 1
  if kCycle == 2 then
    kAfter vaget 0, aFresh
    if abs(kAfter - 0.5) > 0.001 then
      printks "swt fresh late synth reference missed restart: value=%g\n", 0, kAfter
      exitnowk(-1)
    endif
    gkFreshChecks += 1
  endif
endin

instr 4
  if i(gkBeforeChecks) != 1 || i(gkRunningChecks) != 1 || i(gkFreshChecks) != 1 then
    prints "swt late synth restart checks did not complete: %d %d %d\n", i(gkBeforeChecks), i(gkRunningChecks), i(gkFreshChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.03 0.25
i 1 0.03 0.04 0.5
i 2 0.005 0.06
i 3 0.03 0.03
i 4 0.04 0.01
e
</CsScore>
</CsoundSynthesizer>
