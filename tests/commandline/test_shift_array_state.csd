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
  kCycle init 0
  kProcessed init 0
  kWritten init 0
  kFirst init 1
  kSize init p4
  kCycle += 1
  kOffset offsetsmps
  kEarly earlysmps
  kActive = ksmps-kOffset-kEarly
  aPhase phasor 1
  aInput = aPhase*sr+1
  if kCycle == p6 then
    kSize = p5
    kWritten = 0
    kFirst = kProcessed+1
    reinit BUFFER
  endif
BUFFER:
  kFrame[] init i(kSize)
  kFrame shiftin aInput
  rireturn
  kWritten += kActive
  kProcessed += kActive
  kLength lenarray kFrame
  if kLength != max(kSize, ksmps) then
    printks "shiftin output length is wrong\n", 0
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    kExpected = 0
    if kIndex < kWritten then
      kLast = kIndex+1+floor((kWritten-1-kIndex)/kLength)*kLength
      kExpected = kFirst+kLast-1
    endif
    if !(abs(kFrame[kIndex]-kExpected) <= .00001) then
      printks "shiftin cycle %g index %g: got %g, expected %g\n", 0, kCycle, kIndex, kFrame[kIndex], kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  if kProcessed == int(p3*sr) then
    gkChecks += 1
  endif
endin

instr 2
  kCycle init 0
  kProcessed init 0
  kRead init 0
  kSize init p4
  kCycle += 1
  kOffset offsetsmps
  kEarly earlysmps
  kActive = ksmps-kOffset-kEarly
  if kCycle == p7 then
    kSize = p6
    kRead = 0
    reinit BUFFER
  endif
BUFFER:
  kFrame[] init i(kSize)
  kIndex = 0
  while kIndex < kSize do
    kFrame[kIndex] = kIndex+1
    kIndex += 1
  od
  aOutput shiftout kFrame, p5
  rireturn
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    if kIndex >= kOffset && kIndex < ksmps-kEarly then
      ; p8 is the expected integer offset before wrapping.
      kPosition = kRead+kIndex-kOffset+p8
      kExpected = kPosition-floor(kPosition/kSize)*kSize+1
    endif
    kActual vaget kIndex, aOutput
    if kActual != kExpected then
      printks "shiftout cycle %g sample %g: got %g, expected %g\n", 0, kCycle, kIndex, kActual, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kRead += kActive
  kProcessed += kActive
  if kProcessed == int(p3*sr) then
    gkChecks += 1
  endif
endin

; An output without an explicit array init gets one block of storage.
instr 3
  aInput = .25
  kFrame[] shiftin aInput
  kLength lenarray kFrame
  if kLength != ksmps then
    printks "shiftin allocated %g elements, expected %g\n", 0, kLength, ksmps
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    if kFrame[kIndex] != .25 then
      printks "shiftin allocated output index %g: got %g, expected .25\n", 0, kIndex, kFrame[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 22 then
    prints "shift array checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; shiftin: size, new size, reset cycle.
i 1 0 .0625 32 0 0
i 1 0 .0625 47 0 0
i 1 0 .0625 96 48 3
i 1 0 .0625 48 96 3
i 1 0 .0625 3 0 0
i 1 .0006103515625 .0130615234375 47 0 0
i 1 .0006103515625 .0013427734375 32 0 0
; shiftout: size, offset, new size, reset cycle, expected offset.
i 2 0 .0625 32 0 0 0 0
i 2 0 .0625 47 46 0 0 46
i 2 0 .0625 96 100 48 3 100
i 2 0 .0625 48 -1 96 3 -1
i 2 0 .0625 47 -1.75 0 0 -1
i 2 0 .0625 47 -.75 0 0 0
i 2 0 .0625 47 1.75 0 0 1
i 2 0 .0625 64 1e20 0 0 0
i 2 0 .0625 64 -1e20 0 0 0
i 2 .0006103515625 .0130615234375 47 46 0 0 46
i 2 .0006103515625 .0013427734375 32 31 0 0 31
; Reused notes reset their read/write positions.
i 1 .125 .0625 47 0 0
i 2 .125 .0625 47 46 0 0 46
i 3 .25 .01
i 3 .375 .01
i 99 .4 .01
e
</CsScore>
</CsoundSynthesizer>
