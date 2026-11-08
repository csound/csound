<CsTest>
description = "pvsenvftw updates only on new frames, switches destination tables, and preserves guard points"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 6400
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 iFirst ftgen 0, 0, 64, 2, 0
 iSecond ftgen 0, 0, 64, 2, 0
 ; The ordinary entries start at zero. Give each guard point a sentinel.
 tablew .75, 0, iFirst, 0, 0, 2
 tablew .875, 0, iSecond, 0, 0, 2
 tablew 0, 0, iFirst
 tablew 0, 0, iSecond
 kInput[] init 130
 kSource[] init 130
 kExpectedFirst init 0
 kExpectedSecond init 0
 kPreviousFrame init 0
 kCycle init 0
 kCycle += 1
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = .125*(1+kCycle%3)
  kInput[2*kBin+1] = kBin*50
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 kFrame pvs2array kSource, fInput
 ; Table and gain changes between frames must wait for the next update.
 kUseFirst = kCycle%3 == 0
 kTable = kUseFirst == 1 ? iFirst : iSecond
 kGain = 1+kCycle%2
 kUpdated pvsenvftw fInput, kTable, 3, kGain
 kExpectedUpdate = kFrame != kPreviousFrame ? 1 : 0
 if kUpdated != kExpectedUpdate then
  printks "cycle %g: update flag %g expected %g\n", 0, kCycle, kUpdated, kExpectedUpdate
  exitnowk(-1)
 endif
 if kExpectedUpdate == 1 then
  if kUseFirst == 1 then
   kExpectedFirst = kSource[0]*kGain
  else
   kExpectedSecond = kSource[0]*kGain
  endif
 endif
 kPreviousFrame = kFrame
 kBin = 0
 while kBin < 64 do
  kFirst table kBin, iFirst
  kSecond table kBin, iSecond
  if !(abs(kFirst-kExpectedFirst) < .000001 && abs(kSecond-kExpectedSecond) < .000001) then
   printks "cycle %g, bin %g: tables %g/%g expected %g/%g\n", 0, kCycle, kBin, kFirst, kSecond, kExpectedFirst, kExpectedSecond
   exitnowk(-1)
  endif
  kBin += 1
 od
 ; Interpolation halfway past the last ordinary entry reads the guard too.
 kFirstEdge tablei 63.5, iFirst
 kSecondEdge tablei 63.5, iSecond
 if !(abs(kFirstEdge-(kExpectedFirst+.75)/2) < .000001 && abs(kSecondEdge-(kExpectedSecond+.875)/2) < .000001) then
  prints "pvsenvftw changed a guard point\n"
  exitnowk(-1)
 endif
 if kCycle == 24 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 1 then
  prints "pvsenvftw update checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .035
i 99 .04 .00125
e
</CsScore>
</CsoundSynthesizer>
