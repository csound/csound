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

instr 1
  iOffset = int(p2 * sr) % ksmps
  iSamples = int(p3 * sr)
  kCycle init 0
  kProcessed init 0
  kFirst init 1
  kSize init p4
  kCycle += 1
  kOffset = (kCycle == 1 ? iOffset : 0)
  kActive = min(ksmps - kOffset, iSamples - kProcessed)
  aPhase phasor 1
  aInput = aPhase * sr + 1
  if kCycle == p6 then
    kSize = p5
    kFirst = kProcessed + 1
    reinit BUFFER
  endif
BUFFER:
  kFrame[] framebuffer aInput, i(kSize)
  rireturn
  kProcessed += kActive
  kLength lenarray kFrame
  if kLength != kSize then
    printks "framebuffer cycle %g output size %g expected %g\n", 0, kCycle, kLength, kSize
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kSize do
    kValue = kProcessed - kSize + kIndex + 1
    kExpected = (kValue >= kFirst ? kValue : 0)
    if abs(kFrame[kIndex] - kExpected) > .00001 then
      printks "audio-to-frame cycle %g index %g: %g expected %g\n", 0, kCycle, kIndex, kFrame[kIndex], kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
  iOffset = int(p2 * sr) % ksmps
  iSamples = int(p3 * sr)
  kCycle init 0
  kProcessed init 0
  kFirst init 1
  kSize init p4
  kFrame[] init p5
  kCycle += 1
  kOffset = (kCycle == 1 ? iOffset : 0)
  kActive = min(ksmps - kOffset, iSamples - kProcessed)
  kProcessed += kActive
  kIndex = 0
  while kIndex < p5 do
    kFrame[kIndex] = (kCycle - 1) * p5 + kIndex + 1
    kIndex += 1
  od
  if kCycle == p7 then
    kSize = p6
    kFirst = (kCycle - 1) * p5 + 1
    reinit BUFFER
  endif
BUFFER:
  aOutput framebuffer kFrame, i(kSize)
  rireturn
  kIndex = 0
  while kIndex < ksmps do
    kValue = kCycle * p5 - kSize + kIndex - kOffset + 1
    kExpected = (kIndex >= kOffset && kIndex < kOffset + kActive && kValue >= kFirst ? kValue : 0)
    kActual vaget kIndex, aOutput
    if kActual != kExpected then
      printks "frame-to-audio cycle %g sample %g: %g expected %g\n", 0, kCycle, kIndex, kActual, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 15 then
    prints "not all framebuffer checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Audio input: size, new size, reset cycle.
i 1 0 .0625 32 0 0
i 1 0 .0625 47 0 0
i 1 0 .0625 96 48 3
i 1 0 .0625 48 96 3
i 1 .0006103515625 .0130615234375 47 0 0
i 1 .0006103515625 .0013427734375 32 0 0
; Array input: size, frame length, new size, reset cycle.
i 2 0 .0625 64 64 0 0
i 2 0 .0625 47 17 0 0
i 2 0 .0625 96 32 48 3
i 2 0 .0625 48 32 96 3
i 2 0 .0625 32 3 0 0
i 2 .0006103515625 .0130615234375 64 64 0 0
i 2 .0006103515625 .0013427734375 64 64 0 0
; Reuse instances after their first notes end.
i 1 .125 .0625 47 0 0
i 2 .125 .0625 47 17 0 0
i 99 .2 .01
e
</CsScore>
</CsoundSynthesizer>
