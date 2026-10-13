<CsTest>
description = "pvsbandp and pvsbandr follow their boundary gains and transition curves, including Nyquist"

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
 ; Probe both ramps, their endpoints, the flat bands, and Nyquist.
 ; Positions very near a ramp endpoint also exercise steep curves.
 kFrequencies[] fillarray 0, 50, 100, 100.09765625, 125, 150, 175, 199.90234375, 200, 250, 300, 300.09765625, 325, 350, 375, 399.90234375, 400, 450, 3200
 kPassPositions[] fillarray 0, 0, 0, 1/1024, .25, .5, .75, 1023/1024, 1, 1, 1, 1023/1024, .75, .5, .25, 1/1024, 0, 0, 0
 kInput[] init 130
 kBin = 0
 while kBin < lenarray(kFrequencies) do
  kInput[2*kBin] = .25
  kInput[2*kBin+1] = kFrequencies[kBin]*p5
  kBin += 1
 od
 kInput[128] = .25
 kInput[129] = 3200*p5
 fInput pvsfromarray kInput, 32
 fPass pvsbandp fInput, 100, 200, 300, 400, p4
 fReject pvsbandr fInput, 100, 200, 300, 400, p4
 kPass[] init 130
 kReject[] init 130
 kPassFrame pvs2array kPass, fPass
 kRejectFrame pvs2array kReject, fReject
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kBin = 0
  while kBin < lenarray(kFrequencies) do
   kFilter = 0
   while kFilter < 2 do
    kPosition = kFilter == 0 ? kPassPositions[kBin] : 1-kPassPositions[kBin]
    ; Evaluate the documented curve. Near zero it approaches a straight
    ; line; that limit is accurate to better than 2e-9 for these tiny types.
    if abs(p4) < .000001 then
     kGain = kPosition
    elseif p4 > 0 then
     kGain = (exp(p4*(kPosition-1))-exp(-p4))/(1-exp(-p4))
    else
     kGain = (1-exp(p4*kPosition))/(1-exp(p4))
    endif
    kActualAmp = kFilter == 0 ? kPass[2*kBin] : kReject[2*kBin]
    kActualFreq = kFilter == 0 ? kPass[2*kBin+1] : kReject[2*kBin+1]
    if !(abs(kActualAmp-.25*kGain) < .000002) then
     printks "filter %g, type %g, frequency %g: amplitude %g expected %g\n", 0, kFilter, p4, kInput[2*kBin+1], kActualAmp, .25*kGain
     exitnowk(-1)
    endif
    if kActualAmp != 0 && kActualFreq != kInput[2*kBin+1] then
     prints "pvsband changed a retained frequency\n"
     exitnowk(-1)
    endif
    kFilter += 1
   od
   kBin += 1
  od
  if kPass[128] != 0 || kReject[128] != .25 then
   prints "pvsband did not process the Nyquist bin\n"
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr 2
 ; Collapsed ramps define sharp edges. The inner band includes its endpoints.
 iLow = p4 == 0 ? 100 : (p4 == 1 ? 200 : 0)
 iHigh = p4 == 0 ? 400 : (p4 == 1 ? 200 : 3200)
 kInput[] init 130
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = .25
  kInput[2*kBin+1] = kBin*50
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 fPass pvsbandp fInput, iLow, iLow, iHigh, iHigh
 fReject pvsbandr fInput, iLow, iLow, iHigh, iHigh
 kPass[] init 130
 kReject[] init 130
 kPassFrame pvs2array kPass, fPass
 kRejectFrame pvs2array kReject, fReject
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kBin = 0
  while kBin <= 64 do
   kExpectedPass = kBin*50 >= iLow && kBin*50 <= iHigh ? .25 : 0
   if !(abs(kPass[2*kBin]-kExpectedPass) < .000001 && abs(kReject[2*kBin]-(.25-kExpectedPass)) < .000001) then
    printks "sharp band %g..%g Hz, bin %g: pass=%g reject=%g\n", 0, iLow, iHigh, kBin, kPass[2*kBin], kReject[2*kBin]
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 13 then
  prints "pvsband gain checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Curve type, frequency sign.
i 1 0 .0125 0 1
i 1 0 .0125 0 -1
i 1 0 .0125 2.302585093 1
i 1 0 .0125 2.302585093 -1
i 1 0 .0125 -2.302585093 1
i 1 0 .0125 1e-8 1
i 1 0 .0125 -1e-8 1
i 1 0 .0125 .5 1
i 1 0 .0125 1000 1
i 1 0 .0125 -1000 1
; Sharp band, single-frequency band, and a band covering DC through Nyquist.
i 2 0 .0125 0
i 2 0 .0125 1
i 2 0 .0125 2
i 99 .02 .00125
e
</CsScore>
</CsoundSynthesizer>
