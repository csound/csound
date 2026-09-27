<CsTest>
description = "resonxk retains the low-frequency response of serial resonk filters"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkCompleted init 0

instr CompareFilterLayers
  iScaling = p4
  kCycle init 0
  ; Excite the filters once, then compare their decay over many updates.
  kImpulse = (kCycle == 0 ? 1 : 0)
  kFrequency = (kCycle < 1200 ? 1 : 5)
  kBandwidth = (kCycle < 2400 ? 1 : 2)

  kFirst resonk kImpulse, kFrequency, kBandwidth, iScaling
  kSecond resonk kFirst, kFrequency, kBandwidth, iScaling
  kOneLayer resonxk kImpulse, kFrequency, kBandwidth, 1, iScaling
  kTwoLayers resonxk kImpulse, kFrequency, kBandwidth, 2, iScaling

  ; Compare against each reference's peak so zero crossings remain valid.
  kFirstPeak init 0
  kSecondPeak init 0
  kFirstPeak = max(kFirstPeak, abs(kFirst))
  kSecondPeak = max(kSecondPeak, abs(kSecond))
  if !(abs(kOneLayer-kFirst) <= kFirstPeak*.0001) || !(abs(kTwoLayers-kSecond) <= kSecondPeak*.0001) then
    printks "scaling %g, cycle %g: one layer %.12g (expected %.12g), two layers %.12g (expected %.12g)\n", 0, iScaling, kCycle, kOneLayer, kFirst, kTwoLayers, kSecond
    exitnowk -1
  endif
  if kCycle == 4799 then
    gkCompleted += 1
  endif
  kCycle += 1
endin

instr CheckCompletion
  if i(gkCompleted) != 3 then
    prints "All three scaling modes must complete the decay comparison\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Peak scaling, RMS scaling, and no scaling.
i "CompareFilterLayers" 0 .1 1
i "CompareFilterLayers" .1 .1 2
i "CompareFilterLayers" .2 .1 0
i "CheckCompletion" .31 .001
e
</CsScore>
</CsoundSynthesizer>
