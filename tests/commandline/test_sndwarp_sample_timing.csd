<CsTest>
description = "sndwarp overlaps and audio controls respect partial blocks"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  ; Constant tables make every overlap contribute exactly the same sample.
  ; Reused instances also exercise changes in the section allocation size.
  aMono, aClean sndwarp .5, 0, 0, 1, 0, p5, p6, p4, 3, 0
  aLeft, aRight, aCleanLeft, aCleanRight sndwarpst .5, 0, 0, 2, 0, p5, p6, p4, 3, 0
  aGate = 1
  kN = 0
  while kN < ksmps do
    kGate vaget kN, aGate
    kMono vaget kN, aMono
    kClean vaget kN, aClean
    kLeft vaget kN, aLeft
    kRight vaget kN, aRight
    kCleanLeft vaget kN, aCleanLeft
    kCleanRight vaget kN, aCleanRight
    if abs(kMono - kGate*p4*.5) > .00001 || abs(kClean - kGate*.5) > .00001 || \
       abs(kLeft - kGate*p4*.5) > .00001 || abs(kRight - kGate*p4) > .00001 || \
       abs(kCleanLeft - kGate*.5) > .00001 || abs(kCleanRight - kGate) > .00001 then
      printks "sndwarp overlap mismatch: overlap=%g sample=%g mono=%g stereo=%g/%g\n", 0, p4, kN, kMono, kLeft, kRight
      exitnowk(-1)
    endif
    gkChecks += kGate
    kN += 1
  od
endin

instr 2
  ; Compare each audio-rate control with the same value at control rate.
  ; Prefix samples before a sample-accurate note start contain zero.
  aAmp = .5
  aTime = 4/sr
  aSpeed = 1
  kAmp = .5
  kTime = 4/sr
  kSpeed = 1
  aRef, aRefClean sndwarp kAmp, kTime, kSpeed, 4, 0, 4, 0, 2, 3, 1
  aRefL, aRefR, aRefCL, aRefCR sndwarpst kAmp, kTime, kSpeed, 5, 0, 4, 0, 2, 3, 1
  if p4 == 1 then
    aOut, aClean sndwarp aAmp, kTime, kSpeed, 4, 0, 4, 0, 2, 3, 1
    aL, aR, aCL, aCR sndwarpst aAmp, kTime, kSpeed, 5, 0, 4, 0, 2, 3, 1
  elseif p4 == 2 then
    aOut, aClean sndwarp kAmp, aTime, kSpeed, 4, 0, 4, 0, 2, 3, 1
    aL, aR, aCL, aCR sndwarpst kAmp, aTime, kSpeed, 5, 0, 4, 0, 2, 3, 1
  elseif p4 == 3 then
    aOut, aClean sndwarp kAmp, kTime, aSpeed, 4, 0, 4, 0, 2, 3, 1
    aL, aR, aCL, aCR sndwarpst kAmp, kTime, aSpeed, 5, 0, 4, 0, 2, 3, 1
  else
    aOut, aClean sndwarp aAmp, aTime, aSpeed, 4, 0, 4, 0, 2, 3, 1
    aL, aR, aCL, aCR sndwarpst aAmp, aTime, aSpeed, 5, 0, 4, 0, 2, 3, 1
  endif
  aDiff = abs(aRef-aOut) + abs(aRefClean-aClean) + abs(aRefL-aL) + abs(aRefR-aR) + abs(aRefCL-aCL) + abs(aRefCR-aCR)
  aGate = 1
  kN = 0
  while kN < ksmps do
    kDiff vaget kN, aDiff
    kGate vaget kN, aGate
    if kDiff > .00001 then
      printks "sndwarp audio control mismatch: control=%g sample=%g difference=%g\n", 0, p4, kN, kDiff
      exitnowk(-1)
    endif
    gkChecks += kGate
    kN += 1
  od
endin

instr 99
  if i(gkChecks) != 770 then
    prints "sndwarp timing checks did not complete: %g\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 4 -2 1 1 1 1
f 2 0 8 -2 1 2 1 2 1 2 1 2
f 3 0 4 -2 1 1 1 1
f 4 0 16 -2 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
f 5 0 32 -2 1 2 2 4 3 6 4 8 5 10 6 12 7 14 8 16 9 18 10 20 11 22 12 24 13 26 14 28 15 30 16 32
; 77 samples per note, ending three samples before a block boundary.
i 1 0 .0093994140625 1 4 0
i 1 .03125 .0093994140625 3 4 0
i 1 .0625 .0093994140625 2 4 0
i 1 .09375 .0093994140625 2 2 0
i 1 .125 .0093994140625 2 4.5 2.5
; Start three samples into a block, then finish at a block boundary.
i 1 .1566162109375 .0093994140625 2 4 0
i 2 .1878662109375 .0093994140625 1
i 2 .2191162109375 .0093994140625 2
i 2 .2503662109375 .0093994140625 3
i 2 .2816162109375 .0093994140625 4
i 99 .3125 .001
e
</CsScore>
</CsoundSynthesizer>
