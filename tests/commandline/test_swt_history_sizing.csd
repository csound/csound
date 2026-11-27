<CsTest>
description = "swtsynth reconstructs a one-sample analyzer through a larger global consumer block without using overwritten history"

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

instr 1
  setksmps 1
  aInput = 0.2 * oscili(1, 997)
  smallWave@global:swt swtanal aInput, 8, 1, 1, 1, 1, 0, 1, 2
endin

instr 2
  aInput = 0.2 * oscili(1, 997)
  refWave@global:swt swtanal aInput, 8, 1, 1, 1, 1, 0, 1, 2
endin

instr 3
  aSmall swtsynth smallWave
  aRef swtsynth refWave
  kCycle init 0
  kCycle += 1
  if kCycle >= 2 && kCycle <= 10 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aSmall
      kWanted vaget kIndex, aRef
      if abs(kActual - kWanted) > 0.0005 then
        printks "swt history sizing failed: cycle=%d sample=%d got=%g expected=%g\n", 0, kCycle, kIndex, kActual, kWanted
        exitnowk(-1)
      endif
      kIndex += 1
    od
    gkChecks += 1
  endif
endin

instr 4
  if i(gkChecks) != 9 then
    prints "swt history sizing checks did not complete: %d\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
i 2 0 0.2
i 3 0 0.2
i 4 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
