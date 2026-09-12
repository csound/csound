<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef TEST_SR
#define TEST_SR #1024#
#end
sr = $TEST_SR
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0
strset 99, "test_pvsdiskin_stereo.pvx"

instr 1
  if p4 != 1 then
    SFile = "test_pvsdiskin_stereo.pvx"
    iFrames = 8
  else
    SFile = "test_pvsdiskin_single.pvx"
    iFrames = 1
  endif
  iChannel = max(1, min(p5, (p4 != 1 ? 2 : 1))) - 1
  iAmp ftgen 0, 0, 64, -2, 0
  iFreq ftgen 0, 0, 64, -2, 0
  kCount init 0
  kBlock init 0
  kPosition init p6*64
  kSpeed = p7
  if p10 == 2 && kCount >= 3 then
    kSpeed = p7*.5
  endif
  if p10 == 1 && kBlock == 6 then
    kPosition = p6*64
    reinit READ
  endif
READ:
  if p4 == 2 then
    fRead pvsdiskin 99, kSpeed, p9, p6, p5, p8
  else
    fRead pvsdiskin SFile, kSpeed, p9, p6, p5, p8
  endif
  kNew pvsftw fRead, iAmp, iFreq
  rireturn
  if kNew != 0 then
    kPosition = kPosition - iFrames*floor(kPosition/iFrames)
    kFrame = int(kPosition)
    kNext = (kFrame + 1) % iFrames
    kFraction = (p8 == 0 ? 0 : kPosition - kFrame)
    kInterpolatedFrame = kFrame + kFraction*(kNext - kFrame)
    kBin = 0
    while kBin < 33 do
      ; Fixtures contain these exact binary fractions in every bin.
      kExpectedAmp = p9*(.125*(iChannel + 1) + kInterpolatedFrame/64 + kBin/1024)
      kExpectedFreq = 16*kBin + kInterpolatedFrame/8 + iChannel/16
      kAmp table kBin, iAmp
      kFreq table kBin, iFreq
      if !(abs(kAmp - kExpectedAmp) < .000001 && abs(kFreq - kExpectedFreq) < .00001) then
        printks "FAIL pvsdiskin channel=%g offset=%g speed=%g frame=%g bin=%g: (%g,%g) expected (%g,%g)\n", \
            0, p5, p6, p7, kCount, kBin, kAmp, kFreq, kExpectedAmp, kExpectedFreq
        exitnowk -1
      endif
      kBin += 1
    od
    kPosition += kSpeed*1024/sr
    kCount += 1
    if kCount == int(p3*sr/16) then
      gkChecks += 1
    endif
  endif
  kBlock += 1
endin

instr 99
  if i(gkChecks) != 21 then
    prints "FAIL pvsdiskin checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; fixture, channel, seconds offset, speed, interpolation, gain, reinit/modulation.
i 1 0 .125 0 1 .03125 0 1 1 0
i 1 .25 .125 0 2 .03125 0 1 1 0
i 1 .5 .125 0 1 .0390625 .5 1 .5 0
i 1 .75 .125 0 2 .0390625 .5 1 1 0
i 1 1 .125 0 1 .1171875 1 1 1 0
i 1 1.25 .125 0 2 .1171875 1 1 1 0
i 1 1.5 .125 0 1 0 -.5 1 1 0
i 1 1.75 .125 0 2 0 -.5 1 1 0
i 1 2 .125 0 1 -.0078125 0 1 1 0
i 1 2.25 .125 0 2 .1640625 2 0 1 0
i 1 2.5 .125 0 1 0 3.5 1 1 2
i 1 2.75 .125 0 2 .0390625 .5 1 1 1
i 1 3 .125 1 1 0 1 1 1 0
i 1 3.25 .125 1 1 .0390625 -2 1 .5 0
i 1 3.5 .125 1 1 0 1 0 1 1
; Preserve channel clamping.
i 1 3.75 .125 0 0 .03125 0 1 1 0
i 1 4 .125 0 3 .03125 0 1 1 0
i 1 4.25 .125 0 1 0 0 1 0 0
; Numeric filename lookup and independent readers with different lifetimes.
i 1 4.5 .125 2 2 .0390625 .5 1 1 0
i 1 4.75 .0625 0 1 0 1 1 1 0
i 1 4.75 .125 0 2 0 1 1 1 0
i 99 5 .015625
</CsScore>
</CsoundSynthesizer>
