<CsTest>
description = "swtget reports the flat layout and swtset edits and synthesises whole frames"

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
gkSizeChecked init 0
gkNonZero init 0

instr 1
  aInput = 0.25
  w:swt swtanal aInput, 32, 32, 1, 4
  kCoeffs:k[] swtget w, -1
  kZeros:k[] init 128
  wEdited:swt swtset w, kZeros, -1
  aOutput swtsynth wEdited
  kCycle init 0
  kCycle += 1
  if kCycle == 1 then
    if lenarray(kCoeffs) != 128 then
      printks "swtget returned %d coefficients instead of 128\n", 0, lenarray(kCoeffs)
      exitnowk(-1)
    endif
    gkSizeChecked += 1
  endif
  if kCycle >= 2 && kCycle <= 8 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      if abs(kActual) > 0.000001 then
        printks "swtset left a nonzero sample: cycle=%d sample=%d value=%g\n", 0, kCycle, kIndex, kActual
        exitnowk(-1)
      endif
      kIndex += 1
    od
    gkChecks += 1
  endif
endin

instr 2
  aInput = 0.25 * oscili(1, 997)
  wIn:swt swtanal aInput, 32, 32, 1, 4, 4, 0, 0, 1
  kZeros:k[] init 32
  wEdited:swt swtset wIn, kZeros, 0
  aOutput swtsynth wEdited
  kCycle init 0
  kCycle += 1
  if kCycle >= 3 && kCycle <= 8 then
    kIndex = 0
    while kIndex < ksmps do
      kActual vaget kIndex, aOutput
      if abs(kActual) > 0.000001 then
        gkNonZero += 1
      endif
      kIndex += 1
    od
  endif
endin

instr 3
  if i(gkSizeChecked) != 1 || i(gkChecks) != 7 || i(gkNonZero) < 1 then
    prints "swt coefficient editing checks did not complete: %d %d %d\n", i(gkSizeChecked), i(gkChecks), i(gkNonZero)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.2
i 3 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
