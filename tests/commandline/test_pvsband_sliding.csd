<CsTest>
description = "pvsband sliding curves match the documented gains and do not retain clamps between samples"

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
gkChecks init 0

instr 1
 aInput oscili .25, 1024
 fInput pvsanal aInput, 64, 1, 64, 1
 if p5 == 0 then
  aLowcut = 500
  fPass pvsbandp fInput, 500, 1500, 2500, 3500, p4
  fReject pvsbandr fInput, 500, 1500, 2500, 3500, p4
 else
  ; Alternate across the fixed 1500 Hz limit. Clamping the high samples
  ; must not raise that fixed limit on the following low samples.
  aStep phasor sr/2
  aLowcut = 2000-aStep*3000
  fPass pvsbandp fInput, aLowcut, 1500, 2500, 3500, p4
  fReject pvsbandr fInput, aLowcut, 1500, 2500, 3500, p4
 endif
 aInputAmp, aInputFreq pvsbin fInput, 8
 aPassAmp, aPassFreq pvsbin fPass, 8
 aRejectAmp, aRejectFreq pvsbin fReject, 8
 kSample = 0
 while kSample < ksmps do
  kInputAmp vaget kSample, aInputAmp
  kInputFreq vaget kSample, aInputFreq
  kLowcut vaget kSample, aLowcut
  kLowfull = max(1500, kLowcut)
  kFrequency = abs(kInputFreq)
  ; Limit the two linear ramps and take their minimum to form the trapezium.
  if kLowfull == kLowcut then
   kRise = kFrequency >= kLowfull ? 1 : 0
  else
   kRise limit (kFrequency-kLowcut)/(kLowfull-kLowcut), 0, 1
  endif
  kFall limit (3500-kFrequency)/1000, 0, 1
  kPosition = min(kRise, kFall)
  kPassGain = kPosition
  kRejectGain = 1-kPosition
  if p4 != 0 then
   kPassGain = (1-exp(p4*kPosition))/(1-exp(p4))
   kRejectGain = (1-exp(p4*(1-kPosition)))/(1-exp(p4))
  endif
  kPassAmp vaget kSample, aPassAmp
  kRejectAmp vaget kSample, aRejectAmp
  if !(abs(kPassAmp-kInputAmp*kPassGain) < .00001 && abs(kRejectAmp-kInputAmp*kRejectGain) < .00001) then
   printks "sliding type %g, audio control %g, sample %g: pass=%g expected=%g; reject=%g expected=%g\n", 0, p4, p5, kSample, kPassAmp, kInputAmp*kPassGain, kRejectAmp, kInputAmp*kRejectGain
   exitnowk(-1)
  endif
  kSample += 1
 od
 kCycle init 0
 kCycle += 1
 if kCycle == 40 then
  gkChecks += 1
  turnoff
 endif
endin

instr 99
 if i(gkChecks) != 4 then
  prints "pvsband sliding checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Curve type, use an audio-rate low cutoff.
i 1 0 .125 0 0
i 1 0 .125 2.302585093 0
i 1 0 .125 0 1
i 1 0 .125 2.302585093 1
i 99 .15 .01
e
</CsScore>
</CsoundSynthesizer>
