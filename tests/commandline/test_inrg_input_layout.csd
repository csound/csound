<CsTest>
description = "inrg follows input channel count, note offsets and local ksmps"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-i test_input_ramp_16.wav -n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
nchnls_i = 16
0dbfs = 256
gkChecks init 0

instr CheckRange
  setksmps p5
  ; In this fixture, frame n (starting at zero), channel c contains (n+1)*c.
  ; p4 selects the first channel; p5 selects the local block size.
  iStart = round(p2*sr)
  iEnd = iStart+round(p3*sr)
  kBlockStart init floor(iStart/ksmps)*ksmps
  kCycle timeinstk
  kStartChannel = p4 + p6*(kCycle-1)
  aFirst init 0
  aSecond init 0
  inrg kStartChannel, aFirst, aSecond
  kSample = 0
  while kSample < ksmps do
    kFrame = kBlockStart+kSample
    kExpectedFirst = 0
    kExpectedSecond = 0
    if kFrame >= iStart && kFrame < iEnd then
      ; Preserve the existing truncation of fractional channel numbers.
      kExpectedFirst = (kFrame+1)*int(kStartChannel)
      kExpectedSecond = (kFrame+1)*(int(kStartChannel)+1)
    endif
    kFirst vaget kSample, aFirst
    kSecond vaget kSample, aSecond
    if kFirst != kExpectedFirst || kSecond != kExpectedSecond then
      printks "inrg channel=%g frame=%g local ksmps=%g: expected (%g,%g), got (%g,%g)\n", \
        0, kStartChannel, kFrame, ksmps, kExpectedFirst, kExpectedSecond, kFirst, kSecond
      exitnowk -1
    endif
    kSample += 1
  od
  if kBlockStart < iEnd && kBlockStart+ksmps >= iEnd then
    gkChecks += 1
  endif
  kBlockStart += ksmps
endin

opcode LocalRange, aa, k
  setksmps 2
  kStart xin
  aFirst init 0
  aSecond init 0
  inrg kStart, aFirst, aSecond
  xout aFirst, aSecond
endop

instr CheckUDO
  aFirst, aSecond LocalRange 15
  kSample = 0
  while kSample < ksmps do
    kFirst vaget kSample, aFirst
    kSecond vaget kSample, aSecond
    if kFirst != (kSample+1)*15 || kSecond != (kSample+1)*16 then
      printks "inrg UDO frame %g: expected (%g,%g), got (%g,%g)\n", \
        0, kSample, (kSample+1)*15, (kSample+1)*16, kFirst, kSecond
      exitnowk -1
    endif
    kSample += 1
  od
  gkChecks += 1
  turnoff
endin

instr CheckResults
  if i(gkChecks) != 5 then
    prints "inrg input checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; First and last adjacent channel pairs, with mono output and 16 input channels.
i "CheckRange" 0 .25 1 8 0
i "CheckRange" 0 .25 15.75 8 0
; Start one sample into a block and end before the block finishes.
i "CheckRange" .03125 .125 7 8 0
; Four local blocks must advance through the input; change channels each time.
i "CheckRange" 0 .25 4 2 2
i "CheckUDO" 0 .25
i "CheckResults" .25 .03125
e
</CsScore>
</CsoundSynthesizer>
