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

instr 1, 2
  ; Warm and reuse one instance; instrument 2 supplies a fresh comparison.
  kCycle init 0
  kCycle += 1
  kInput[] init 1026
  kOutput[] init 1026
  fInput pvsosc .5, 512, 4, 1024, 256
  fFrozen pvsfreeze fInput, p4, p5
  kInputFrame pvs2tab kInput, fInput
  kOutputFrame pvs2tab kOutput, fFrozen
  kIndex = 0
  while kIndex < 1026 do
    kExpectedAmp = (p4 >= 1 ? 0 : kInput[kIndex])
    kExpectedFreq = (p5 >= 1 ? 0 : kInput[kIndex + 1])
    if kOutput[kIndex] != kExpectedAmp || kOutput[kIndex + 1] != kExpectedFreq then
      printks "pvsfreeze note at %g retained an earlier spectrum in bin %g\n", 0, p2, kIndex / 2
      exitnowk -1
    endif
    kIndex += 2
  od
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

opcode FreezeReference, a, aaaa
  setksmps 1
  aInput, aFreezeA, aFreezeF, aReset xin
  kFreezeA downsamp aFreezeA
  kFreezeF downsamp aFreezeF
  kReset downsamp aReset
  fInput pvsanal aInput, 64, 1, 64, 1
  if kReset != 0 then
    reinit FREEZE
  endif
FREEZE:
  fFrozen pvsfreeze fInput, kFreezeA, kFreezeF
  rireturn
  aOutput pvsynth fFrozen
  xout aOutput
endop

instr 3, 4
  kCycle init 0
  kCycle += 1
  aTone oscili .25, 3072
  aInput = 1 + aTone
  kFreeze = (p4 == 3 || (kCycle >= 3 && kCycle < 7) ? 2 : .5)
  kFreezeA = (p4 == 2 ? .5 : kFreeze)
  kFreezeF = (p4 == 1 ? .5 : kFreeze)
  fInput pvsanal aInput, 64, 1, 64, 1
  aReset = 0
  if kCycle == p5 then
    vaset 1, 0, aReset
    reinit FREEZE
  endif
FREEZE:
  fFrozen pvsfreeze fInput, kFreezeA, kFreezeF
  rireturn
  aActual pvsynth fFrozen
  aFreezeA = kFreezeA
  aFreezeF = kFreezeF
  aExpected FreezeReference aInput, aFreezeA, aFreezeF, aReset
  kFirst = (kCycle == 1 ? p6 : 0)
  kEnd = (kCycle == p8 ? p7 : ksmps)
  kIndex = 0
  while kIndex < ksmps do
    kActual vaget kIndex, aActual
    kExpected vaget kIndex, aExpected
    if kIndex < kFirst || kIndex >= kEnd || p4 == 3 || (p4 != 2 && p5 > 0 && kCycle >= p5 && kCycle < 7) then
      kExpected = 0
    endif
    if abs(kActual - kExpected) > .00001 * (1 + abs(kExpected)) then
      printks "pvsfreeze mode %g cycle %g sample %g: got %g, expected %g\n", 0, p4, kCycle, kIndex, kActual, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == p8 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 16 then
    prints "not all pvsfreeze state checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Amplitude/frequency freeze flags for fresh and reused frame-based streams.
i 1 0 .03125 0 0
i 1 .0625 .03125 1 1
i 2 .0625 .03125 1 1
i 1 .125 .03125 0 0
i 1 .1875 .03125 1 0
i 1 .25 .03125 0 0
i 1 .3125 .03125 0 1
; Sliding mode: switch mode, reinit block, first/end samples, block count.
i 3 0 .03125 0 5 0 32 8
i 3 .0625 .03125 3 0 0 32 8
i 4 .0625 .03125 3 0 0 32 8
i 3 .125 .03125 1 5 0 32 8
i 3 .1875 .03125 2 5 0 32 8
i 3 .2506103515625 .0130615234375 0 0 5 16 4
i 3 .2740478515625 .0130615234375 1 0 5 16 4
i 3 .2974853515625 .0130615234375 2 0 5 16 4
i 3 .3131103515625 .0013427734375 0 0 5 16 1
i 99 .36 .01
e
</CsScore>
</CsoundSynthesizer>
