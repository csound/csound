<CsTest>
description = "pvsfromarray and tab2pvs publish current array data only when a frame is due"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkCompleted init 0

instr CheckTiming
  iHop = (p4 == 0 ? 32 : int(p4))
  kCycle init 0
  kCycle += 1
  kInterleaved[] init 130
  kMagnitudes[] init 65
  kFrequencies[] init 65
  ; Label every bin with the current cycle and its own frequency.
  kBin = 0
  while kBin < 65 do
    kInterleaved[2*kBin] = kCycle + kBin/128
    kInterleaved[2*kBin + 1] = 64*kBin
    kMagnitudes[kBin] = kInterleaved[2*kBin]
    kFrequencies[kBin] = kInterleaved[2*kBin + 1]
    kBin += 1
  od
  if p5 == 1 && kCycle == 10 then
    reinit CONVERT
  endif
CONVERT:
  fInterleaved pvsfromarray kInterleaved, p4
  fSplit pvsfromarray kMagnitudes, kFrequencies, p4
  fAliasInterleaved tab2pvs kInterleaved, p4
  fAliasSplit tab2pvs kMagnitudes, kFrequencies, p4
  rireturn
  kOut1[] init 130
  kOut2[] init 130
  kOut3[] init 130
  kOut4[] init 130
  kFrame1 pvs2array kOut1, fInterleaved
  kFrame2 pvs2array kOut2, fSplit
  kFrame3 pvs2array kOut3, fAliasInterleaved
  kFrame4 pvs2array kOut4, fAliasSplit
  kStartCycle = (p5 == 1 && kCycle >= 10 ? 10 : 1)
  kExpectedFrame = 1 + int((kCycle - kStartCycle)*ksmps/iHop)
  kCopiedCycle = kStartCycle + ceil((kExpectedFrame - 1)*iHop/ksmps)
  if kFrame1 != kExpectedFrame || kFrame2 != kExpectedFrame || \
     kFrame3 != kExpectedFrame || kFrame4 != kExpectedFrame then
    printks "hop %g cycle %g: frames %g/%g/%g/%g, expected %g\n", 0, p4, kCycle, kFrame1, kFrame2, kFrame3, kFrame4, kExpectedFrame
    exitnowk(-1)
  endif
  kBin = 0
  while kBin < 65 do
    kExpectedAmp = kCopiedCycle + kBin/128
    kExpectedFreq = 64*kBin
    if kOut1[2*kBin] != kExpectedAmp || kOut1[2*kBin+1] != kExpectedFreq || \
       kOut2[2*kBin] != kExpectedAmp || kOut2[2*kBin+1] != kExpectedFreq || \
       kOut3[2*kBin] != kExpectedAmp || kOut3[2*kBin+1] != kExpectedFreq || \
       kOut4[2*kBin] != kExpectedAmp || kOut4[2*kBin+1] != kExpectedFreq then
      printks "hop %g cycle %g bin %g: wrong copied data\n", 0, p4, kCycle, kBin
      exitnowk(-1)
    endif
    kBin += 1
  od
  if kCycle == 32 then
    gkCompleted += 1
  endif
endin

instr CheckCompletion
  if i(gkCompleted) != 9 then
    prints "Not all array-to-PVS timing checks finished\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Exact control-block hops, nonmultiples, truncation, defaults and reinit.
i "CheckTiming" 0 .0625 16 0
i "CheckTiming" 0 .0625 32 0
i "CheckTiming" 0 .0625 40 0
i "CheckTiming" 0 .0625 17 0
i "CheckTiming" 0 .0625 63 0
i "CheckTiming" 0 .0625 40.75 0
i "CheckTiming" 0 .0625 0 0
i "CheckTiming" 0 .0625 40 1
i "CheckTiming" 0 .0625 32 1
i "CheckCompletion" .08 .01
e
</CsScore>
</CsoundSynthesizer>
