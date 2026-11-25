<CsTest>
description = "swtanal and swtsynth reconstruct direct and overlap-add frames with the documented latency"

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

instr 2
  if i(gkChecks) != 18 then
    prints "swt reconstruction checks did not complete: %d\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.2 32 32 0 1
i 1 0 0.2 64 16 1 2
i 2 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
