<CsTest>
description = "Sliding pvsynth processes active samples and clears both ends of partial blocks"

[expect]
exit = 0
</CsTest>
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

opcode SampleReference, a, a
 setksmps 1
 aInput xin
 fAnalysis pvsanal aInput, 66, 1, 66, 1
 aOutput pvsynth fAnalysis
 xout aOutput
endop

instr CheckBlock
 aTone oscili .25, 137
 aInput = .25 + aTone
 fAnalysis pvsanal aInput, 66, 1, 66, 1
 aOutput = -2
 aOutput pvsynth fAnalysis
 aReference SampleReference aInput
 kCycle init 0
 kCycle += 1
 kFirst = (kCycle == 1 ? p4 : 0)
 kEnd = (kCycle == p6 ? p5 : ksmps)
 kIndex = 0
 while kIndex < ksmps do
  kActual vaget kIndex, aOutput
  kExpected vaget kIndex, aReference
  ; Outside the note, silence must replace any old frame contents.
  if kIndex < kFirst || kIndex >= kEnd then
   kExpected = 0
  endif
  if !(abs(kActual-kExpected) < .0001) then
   printks "pvsynth partial block: cycle=%g sample=%g active=[%g,%g) got=%g expected=%g\n", 0, kCycle, kIndex, kFirst, kEnd, kActual, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kCycle == p6 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 5 then
  prints "pvsynth partial-block checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; First active sample, end sample in last block, block count.
i "CheckBlock" 0 .015625 0 32 4
i "CheckBlock" .0006103515625 .0130615234375 5 16 4
i "CheckBlock" .0006103515625 .0013427734375 5 16 1
i "CheckBlock" 0 .013671875 0 16 4
i "CheckBlock" 0 .001953125 0 16 1
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
