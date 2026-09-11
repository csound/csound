<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; Compute each sliding spectrum separately as a reference for block processing.
opcode Moments, aa, a
  setksmps 1
  aInput xin
  fSig pvsanal aInput, 128, 1, 128, 0
  kCenter pvscent fSig
  kWidth pvsbandwidth fSig
  aCenter = kCenter
  aWidth = kWidth
  xout aCenter, aWidth
endop

instr 1
  kFrame[] init 130
  kCycle init 0
  kCycle += 1
  ; Silence after a nonzero frame must clear the cached measurement.
  kScale = kCycle < 10 ? p8 : 0
  kFrame[p4*2] = p6*kScale
  kFrame[p4*2+1] = p4*sr/128
  kFrame[p5*2] = p7*kScale
  kFrame[p5*2+1] = p5*sr/128
  fSig pvsfromarray kFrame, 32
  kCenter pvscent fSig
  aCenter pvscent fSig
  kWidth pvsbandwidth fSig
  kAudio downsamp aCenter
  iCenter = (p4*p6+p5*p7)/(p6+p7)*sr/128
  iWidth = sqrt((p6*(p4*sr/128-iCenter)^2+p7*(p5*sr/128-iCenter)^2)/(p6+p7))
  if kCycle == 8 || kCycle == 16 then
    kExpectedCenter = kCycle == 8 ? iCenter : 0
    kExpectedWidth = kCycle == 8 ? iWidth : 0
    if !(abs(kCenter-kExpectedCenter) < .01 && abs(kWidth-kExpectedWidth) < .01 && abs(kAudio-kCenter) < .01) then
      printks "spectral moments: centroid %g, bandwidth %g, audio centroid %g\n", 0, kCenter, kWidth, kAudio
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin

instr 2
  ; Changing spectra catch sums carried from one sample to the next.
  aInput oscili .1, 370
  aOther oscili .07, 1730
  aInput += aOther
  fSig pvsanal aInput, 128, 1, 128, 0
  fLouder pvsgain fSig, 4
  aCenter pvscent fSig
  kCenter pvscent fSig
  kWidth pvsbandwidth fSig
  kLouder pvsbandwidth fLouder
  aRefCenter, aRefWidth Moments aInput
  kRefCenter downsamp aRefCenter
  kRefWidth downsamp aRefWidth
  kError max_k abs(aCenter-aRefCenter), 1, 1
  if !(kError < .05 && abs(kCenter-kRefCenter) < .05 && abs(kWidth-kRefWidth) < .05 && abs(kLouder-kWidth) < .05) then
    printks "sliding moments: audio error %g, centroid %g/%g, bandwidth %g/%g, scaled %g\n", 0, kError, kCenter, kRefCenter, kWidth, kRefWidth, kLouder
    exitnowk(-1)
  endif
  kOnce init 1
  if kOnce == 1 then
    gkChecks += 1
    kOnce = 0
  endif
endin

instr 3
  kFrame[] init 130
  kFrame[32] = 1
  fSig pvsfromarray kFrame, 32
  kCenter pvscent fSig
  aCenter = -1
  aCenter pvscent fSig
  kCycle init 0
  kIndex = 0
  while kIndex < ksmps do
    kSample vaget kIndex, aCenter
    kPosition = kCycle*ksmps+kIndex
    kExpected = kPosition < 5 || kPosition >= 45 ? 0 : kCenter
    if !(abs(kSample-kExpected) < .01) then
      printks "centroid partial block: sample %g, expected %g, got %g\n", 0, kPosition, kExpected, kSample
      exitnowk(-1)
    endif
    kIndex += 1
  od
  kCycle += 1
  if kCycle == 3 then
    gkChecks += 1
  endif
endin

instr 4
  ; A rectangular window gives two exact bins after the first 128 samples.
  aLow oscili .1*p4, 500
  aHigh oscili .3*p4, 1500
  fSig pvsanal aLow+aHigh, 128, 1, 128, 9
  kCenter pvscent fSig
  kWidth pvsbandwidth fSig
  kCycle init 0
  kCycle += 1
  if kCycle == 12 then
    if !(abs(kCenter-1250) < .05 && abs(kWidth-sqrt(187500)) < .05) then
      printks "sliding two-bin spectrum: centroid %g, bandwidth %g\n", 0, kCenter, kWidth
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 17 then
    prints "spectral moment checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Bin positions, magnitudes, overall level.
i 1 0 .034 16 48 1 1 1
i 1 0 .034 16 48 1 1 4
i 1 0 .034 0 64 1 3 1
i 1 0 .034 0 64 1 3 .01
i 1 0 .034 0 64 1 0 1
i 1 0 .034 0 64 0 1 1
i 2 .04 .04
i 2 .080625 .039
i 3 .140625 .005
i 4 .15 .026 1
i 4 .15 .026 4
i 99 .18 .002
</CsScore>
</CsoundSynthesizer>
