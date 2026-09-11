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

opcode BinReference, aa, ai
  setksmps 1
  aInput, iBin xin
  fInput pvsanal aInput, 64, 1, 64, 1
  kAmp, kFreq pvsbin fInput, iBin
  aAmp = kAmp
  aFreq = kFreq
  xout aAmp, aFreq
endop

instr 1
  ; Accept bin changes only when a new PVS frame arrives.
  kCycle init 0
  kCycle += 1
  kBins[] init 1026
  fInput pvsosc .5, p4, 4, 1024, 256
  kFrame pvs2tab kBins, fInput
  kBin = (kCycle % 3 == 1 ? 64.75 : (kCycle % 3 == 2 ? 128 : 512.75))
  kLastFrame init 0
  kExpectedAmp init 0
  kExpectedFreq init 0
  if kFrame > kLastFrame then
    kExpectedAmp = kBins[2 * int(kBin)]
    kExpectedFreq = kBins[2 * int(kBin) + 1]
    kLastFrame = kFrame
  endif
  ; Overwrite both buffers so stale samples cannot pass as held values.
  aAmp = -1
  aFreq = -1
  aAmp, aFreq pvsbin fInput, kBin
  kAmp, kFreq pvsbin fInput, kBin
  if kAmp != kExpectedAmp || kFreq != kExpectedFreq then
    printks "pvsbin control output missed bin %g on cycle %g\n", 0, kBin, kCycle
    exitnowk -1
  endif
  kFirst = (kCycle == 1 ? p5 : 0)
  kEnd = (kCycle == p7 ? p6 : ksmps)
  kIndex = 0
  while kIndex < ksmps do
    kActualAmp vaget kIndex, aAmp
    kActualFreq vaget kIndex, aFreq
    kWantAmp = (kIndex >= kFirst && kIndex < kEnd ? kExpectedAmp : 0)
    kWantFreq = (kIndex >= kFirst && kIndex < kEnd ? kExpectedFreq : 0)
    if kActualAmp != kWantAmp || kActualFreq != kWantFreq then
      printks "pvsbin frame cycle %g sample %g: amp %g/%g, freq %g/%g\n", 0, kCycle, kIndex, kActualAmp, kWantAmp, kActualFreq, kWantFreq
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == p7 then
    gkChecks += 1
  endif
endin

instr 2
  ; A sliding stream must expose every sample, not only the block's first.
  kCycle init 0
  kCycle += 1
  aTone oscili .25, 3072
  aInput = 1 + aTone
  fInput pvsanal aInput, 64, 1, 64, 1
  aAmp, aFreq pvsbin fInput, p4
  kAmp, kFreq pvsbin fInput, p4
  aExpectedAmp, aExpectedFreq BinReference aInput, p4
  kFirst = (kCycle == 1 ? p5 : 0)
  kEnd = (kCycle == p7 ? p6 : ksmps)
  kIndex = 0
  while kIndex < ksmps do
    kActualAmp vaget kIndex, aAmp
    kActualFreq vaget kIndex, aFreq
    kWantAmp vaget kIndex, aExpectedAmp
    kWantFreq vaget kIndex, aExpectedFreq
    if kIndex < kFirst || kIndex >= kEnd then
      kWantAmp = 0
      kWantFreq = 0
    endif
    if abs(kActualAmp - kWantAmp) > .000001 * (1 + abs(kWantAmp)) || abs(kActualFreq - kWantFreq) > .000001 * (1 + abs(kWantFreq)) then
      printks "pvsbin sliding bin %g cycle %g sample %g: amp %g/%g, freq %g/%g\n", 0, p4, kCycle, kIndex, kActualAmp, kWantAmp, kActualFreq, kWantFreq
      exitnowk -1
    endif
    if kIndex == kFirst && (kAmp != kActualAmp || kFreq != kActualFreq) then
      printks "pvsbin control output did not use the first active sample\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == p7 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "not all pvsbin output checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Frequency/bin, first sample, final sample, number of blocks.
i 1 0 .0625 512 0 32 16
i 1 0 .0625 4096 0 32 16
i 1 .0006103515625 .0130615234375 512 5 16 4
i 1 .0006103515625 .0013427734375 512 5 16 1
i 2 0 .0625 .75 0 32 16
i 2 0 .0625 24.75 0 32 16
i 2 0 .0625 32.75 0 32 16
i 2 .0006103515625 .0130615234375 24.75 5 16 4
i 2 .0006103515625 .0130615234375 32.75 5 16 4
i 2 .0006103515625 .0013427734375 24.75 5 16 1
i 99 .07 .01
e
</CsScore>
</CsoundSynthesizer>
