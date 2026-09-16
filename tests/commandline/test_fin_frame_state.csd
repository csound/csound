<CsTest>
description = "fin and fink preserve frames, channel layout, EOF silence and partial blocks"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 2
0dbfs = 1
gkChecks init 0

instr 1
  setksmps 1
  kFrame init 1
  aLeft = kFrame / 32768
  aRight = -aLeft
  fout "fin_frames.wav", 14, aLeft, aRight
  fout "fin_frames.raw", 36, aLeft, aRight
  kFrame += 1
endin

instr 2
  iOffset = int(p2*sr+.5) % ksmps
  iLength = int(p3*sr+.5)
  kCount init 0
  aLeft init 0
  aRight init 0
  fin "fin_frames.wav", p4, 0, aLeft, aRight
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart+iLength-kCount)
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kFrame = p4 + kCount + kN - kStart + 1
      if kFrame <= 527 then
        kExpected = kFrame / 32768
      endif
    endif
    kLeft vaget kN, aLeft
    kRight vaget kN, aRight
    if abs(kLeft-kExpected) > 1e-7 || abs(kRight+kExpected) > 1e-7 then
      printks "scalar frame %g sample %g: expected %g, got %g %g\n", 0, kCount, kN, kExpected, kLeft, kRight
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd-kStart
  if kCount == iLength then
    gkChecks += 1
  endif
endin

instr 3
  iOffset = int(p2*sr+.5) % ksmps
  iLength = int(p3*sr+.5)
  kCount init 0
  aData[] init (p5 == -2 ? 2 : 1)
  if p5 == -2 then
    fin "fin_frames.raw", p4, -2, aData
  else
    fin "fin_frames.wav", p4, 0, aData
  endif
  if lenarray(aData) != 2 then
    exitnow -1
  endif
  aLeft = aData[0]
  aRight = aData[1]
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart+iLength-kCount)
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kFrame = p4 + kCount + kN - kStart + 1
      if kFrame <= 527 then
        kExpected = kFrame / 32768
      endif
    endif
    kLeft vaget kN, aLeft
    kRight vaget kN, aRight
    if abs(kLeft-kExpected) > 1e-7 || abs(kRight+kExpected) > 1e-7 then
      printks "array frame %g sample %g: expected %g, got %g %g\n", 0, kCount, kN, kExpected, kLeft, kRight
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd-kStart
  if kCount == iLength then
    gkChecks += 1
  endif
endin

instr 4
  setksmps 1
  kLeft init 0
  kRight init 0
  kCount init 0
  fink "fin_frames.wav", p4, 0, kLeft, kRight
  kFrame = p4 + kCount + 1
  kExpected = (kFrame <= 527 ? kFrame/32768 : 0)
  if abs(kLeft-kExpected) > 1e-7 || abs(kRight+kExpected) > 1e-7 then
    printks "fink frame %g: expected %g, got %g %g\n", 0, kFrame, kExpected, kLeft, kRight
    exitnowk -1
  endif
  kCount += 1
  if kCount == 560 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 7 then
    prints "fin frame checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; 527 stereo frames, so the last read is short.
i1 0 0.5146484375
; Simultaneous readers have independent positions in the shared file.
i2 1.0029296875 0.546875 10
i2 1.0029296875 0.546875 20
i3 1.0029296875 0.546875 10 0
i3 1.0029296875 0.546875 10 -2
i4 1.0029296875 0.546875 10
; Reused instances and a start beyond EOF.
i2 2.0029296875 0.0263671875 600
i3 2.0029296875 0.0263671875 600 0
i99 2.1 .01
</CsScore>
</CsoundSynthesizer>
