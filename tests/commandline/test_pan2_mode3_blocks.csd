<CsTest>
description = "pan2 mode 3 sweeps correctly through full and partial blocks"

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
nchnls = 2
0dbfs = 1
gkChecks init 0

instr CheckSweep
  iHalfPi = 2*taninv(1)
  aInput = .5
  ; Sweep across the stereo field in 64 samples.
  aPan phasor sr/64
  aLeft, aRight pan2 aInput, aPan, 3
  aPair[] pan2 aInput, aPan, 3
  aExpectedLeft = aInput*cos(iHalfPi*aPan)
  aExpectedRight = aInput*sin(iHalfPi*aPan)

  ; Change the control-rate pan once per block as well.
  kBlock init 0
  kPan = (kBlock % 3)/2
  aControlLeft, aControlRight pan2 aInput, kPan, 3
  aExpectedControlLeft = aInput*cos(iHalfPi*kPan)
  aExpectedControlRight = aInput*sin(iHalfPi*kPan)

  ; Check every output sample, including the inactive parts of each block.
  kSample = 0
  while kSample < ksmps do
    kLeft vaget kSample, aLeft
    kRight vaget kSample, aRight
    kPairLeft vaget kSample, aPair[0]
    kPairRight vaget kSample, aPair[1]
    kExpectedLeft vaget kSample, aExpectedLeft
    kExpectedRight vaget kSample, aExpectedRight
    kControlLeft vaget kSample, aControlLeft
    kControlRight vaget kSample, aControlRight
    kExpectedControlLeft vaget kSample, aExpectedControlLeft
    kExpectedControlRight vaget kSample, aExpectedControlRight
    kError = abs(kLeft-kExpectedLeft) + abs(kRight-kExpectedRight)
    kError += abs(kPairLeft-kExpectedLeft) + abs(kPairRight-kExpectedRight)
    kError += abs(kControlLeft-kExpectedControlLeft) + abs(kControlRight-kExpectedControlRight)
    if !(kError < .00001) then
      printks "pan2 mode 3 start=%g block=%g sample=%g error=%g\n", \
        0, p2*sr, kBlock, kSample, kError
      exitnowk(-1)
    endif
    kSample += 1
  od
  if kBlock == 0 then
    gkChecks += 1
  endif
  kBlock += 1
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "pan2 completed %g sweeps; expected 2\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, then a note that starts and ends inside a block.
i "CheckSweep" 0 [64/8192]
i "CheckSweep" [(80+3)/8192] [60/8192]
i "CheckResults" [160/8192] [16/8192]
e
</CsScore>
</CsoundSynthesizer>
