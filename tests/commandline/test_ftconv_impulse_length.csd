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
  iChannels = p1
  iIR ftgen 0, 0, 32*iChannels, -2, 0
  iFrame = 0
  while iFrame < 32 do
    tableiw iFrame+1, iFrame*iChannels, iIR
    if iChannels == 2 then
      tableiw -(iFrame+1)*.5, iFrame*2+1, iIR
    endif
    iFrame += 1
  od
  iLength = 32-p5
  if p6 > 0 then
    iLength = min(iLength, p6)
  endif
  aInput = .1
  if iChannels == 1 then
    aLeft ftconv aInput, iIR, p4, p5, p6
    aRight = -.5*aLeft
  else
    aLeft, aRight ftconv aInput, iIR, p4, p5, p6
  endif
  kProcessed init 0
  kOffset offsetsmps
  kEarly earlysmps
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    if kIndex >= kOffset && kIndex < ksmps-kEarly then
      kTime = kProcessed+kIndex-kOffset-p4
      kTap = 0
      while kTap < iLength && kTap <= kTime do
        kSource = p5+kTap
        if kSource >= 0 && kSource < 32 then
          kExpected += .1*(kSource+1)
        endif
        kTap += 1
      od
    endif
    kLeft vaget kIndex, aLeft
    kRight vaget kIndex, aRight
    if !(abs(kLeft-kExpected) <= .0001 && abs(kRight+.5*kExpected) <= .0001) then
      printks "ftconv channels %g partition %g skip %g length %g sample %g: got %g / %g, expected %g / %g\n", 0, p1, p4, p5, p6, kProcessed+kIndex, kLeft, kRight, kExpected, -.5*kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kProcessed += ksmps-kOffset-kEarly
  if kProcessed == int(p3*sr) then
    gkChecks += 1
  endif
endin

; Two different stereo partition layouts can require the same byte count.
instr 3
  iIR ftgen 0, 0, 128, -7, 1, 128, 1
  kCycle init 0
  kAge init 0
  kPart init p4
  kLength init p5
  if kCycle == 3 then
    kPart = p6
    kLength = p7
    if p4 != p6 || p5 != p7 || p8 == 0 then
      kAge = 0
    endif
    reinit CONVOLUTION
  endif
  aInput = .1
CONVOLUTION:
  aLeft, aRight ftconv aInput, iIR, i(kPart), 0, i(kLength), p8
  rireturn
  kIndex = 0
  while kIndex < ksmps do
    kExpected = .1*max(0, min(kLength, kAge+kIndex-kPart+1))
    kLeft vaget kIndex, aLeft
    kRight vaget kIndex, aRight
    if !(abs(kLeft-kExpected) <= .00001 && abs(kRight-kExpected) <= .00001) then
      printks "ftconv reinit cycle %g part %g length %g sample %g: got %g / %g, expected %g\n", 0, kCycle, kPart, kLength, kIndex, kLeft, kRight, kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kAge += ksmps
  kCycle += 1
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 20 then
    prints "ftconv checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Partition size, skipped frames, impulse length.
i 1 0 .03125 4 0 5
i 2 0 .03125 4 0 5
i 1 0 .03125 8 0 1
i 1 0 .03125 8 0 8
i 1 0 .03125 8 3 9
i 1 0 .03125 16 7 17
i 1 0 .03125 8 -3 9
i 1 0 .03125 8 0 0
i 2 0 .03125 8 0 1
i 2 0 .03125 8 0 8
i 2 0 .03125 8 3 9
i 2 0 .03125 16 7 17
i 2 0 .03125 8 -3 9
i 2 0 .03125 8 0 0
i 1 .0006103515625 .0130615234375 8 3 9
i 2 .0006103515625 .0130615234375 8 3 9
; Initial and new partition/length, skip initialization flag.
i 3 .125 .03125 8 33 16 17 1
i 3 .125 .03125 16 17 8 33 1
i 3 .125 .03125 8 33 8 33 1
i 3 .125 .03125 8 33 8 33 0
i 99 .25 .01
e
</CsScore>
</CsoundSynthesizer>
