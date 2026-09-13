<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 2
0dbfs = 1
gkChecks init 0

instr 1
  iLength = p4
  iEnvelope = p5
  iRepeats = p6
  iBarLength = iLength*8/sr
  iRamp = (iEnvelope == 0 ? 0 : min(64, int(iLength/4)))
  iSamples = round(p3*sr)
  kStart init round(p2*sr) % ksmps
  kSample init 0
  aOne upsamp 1
  aMono bbcutm aOne, 4, 2, iBarLength, 1, iRepeats, 1, 0, iEnvelope
  aLeft upsamp 1
  aRight upsamp 2
  aLeft, aRight bbcuts aLeft, aRight, 4, 2, iBarLength, 1, iRepeats, 1, 0, iEnvelope
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    if kIndex >= kStart && kSample < iSamples then
      kPosition = kSample % iLength
      kExpected = 1
      if iRamp > 0 then
        if kPosition < iRamp then
          kExpected = (exp(kPosition/iRamp) - 1)/(exp(1) - 1)
        elseif kPosition >= iLength - iRamp then
          kExpected = (exp((iLength - kPosition)/iRamp) - 1)/(exp(1) - 1)
        endif
      endif
      kSample += 1
    endif
    kMono vaget kIndex, aMono
    kLeft vaget kIndex, aLeft
    kRight vaget kIndex, aRight
    kError = abs(kMono - kExpected) + abs(kLeft - kExpected) + abs(kRight - 2*kExpected)
    if !(kError < .000002) then
      printks "bbcut envelope mismatch: length=%g sample=%g expected=%g mono=%g left=%g right=%g\n", 0, iLength, kSample, kExpected, kMono, kLeft, kRight
      exitnowk(-1)
    endif
    kIndex += 1
  od
  kStart = 0
  kChecked init 0
  if kSample == iSamples && kChecked == 0 then
    gkChecks += 1
    kChecked = 1
  endif
endin

instr 2
  ; Each one-second phrase ends in short stutters. The next phrase must
  ; restore its 64-sample ramp, whatever random cuts preceded the stutter.
  aOne upsamp 1
  aMono bbcutm aOne, 4, 8, 4, 1, 0, 8, 1, 1
  aLeft, aRight bbcuts aOne, aOne, 4, 8, 4, 1, 0, 8, 1, 1
  kSample init 0
  kIndex = 0
  while kIndex < ksmps do
    kPosition = kSample % sr
    if kPosition < 64 then
      kExpected = (exp(kPosition/64) - 1)/(exp(1) - 1)
      kMono vaget kIndex, aMono
      kLeft vaget kIndex, aLeft
      kRight vaget kIndex, aRight
      kError = abs(kMono - kExpected) + abs(kLeft - kExpected) + abs(kRight - kExpected)
      if !(kError < .000002) then
        printks "bbcut retained a short ramp: sample=%g expected=%g mono=%g stereo=%g\n", 0, kSample, kExpected, kMono, kLeft
        exitnowk(-1)
      endif
      if kPosition == 63 then
        gkChecks += 1
      endif
    endif
    kSample += 1
    kIndex += 1
  od
endin

instr 99
  if i(gkChecks) != 9 then
    prints "bbcut envelope checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full and partial blocks, shortened ramps, repeats, and envelopes disabled.
i 1 0 .0625 256 1 0
i 1 .12548828125 .0628662109375 64 1 4
i 1 .2508544921875 .03125 16 1 0
i 1 .37548828125 .03125 3 1 4
i 1 .5 .0625 64 0 4
i 1 .625 .5 1024 1 4
i 2 1.25 2.03125
i 99 3.5 .01
e
</CsScore>
</CsoundSynthesizer>
