<CsTest>
description = "phasorbnk keeps rounded output in range without resetting its phase"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 65536
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckRange
  kFrequency = p4
  aFrequency = kFrequency
  kBank phasorbnk kFrequency, 0, 2, p5
  aBankK phasorbnk kFrequency, 0, 2, p5
  aBankA phasorbnk aFrequency, 0, 2, p5
  kAudioK downsamp aBankK
  kAudioA downsamp aBankA
  if !(kBank >= 0 && kBank < 1 && kAudioK >= 0 && kAudioK < 1 && kAudioA >= 0 && kAudioA < 1) then
    printks "phasorbnk frequency=%g: expected [0, 1), got k %.12g, ak %.12g, aa %.12g\n", 0, p4, kBank, kAudioK, kAudioA
    exitnowk -1
  endif
  if timeinstk() == 16 then
    gkChecks += 1
    turnoff
  endif
endin

instr CheckPhaseIsKept
  ; These powers of two are exact in both builds. Start 2^-24 below one
  ; and advance by 2^-26 per sample. Float output rounds to one before
  ; the internal phase reaches one, but that must not reset the phase.
  iStart = 1 - 1/16777216
  iStep = 1/67108864
  kFrequency = iStep*sr
  aFrequency = kFrequency
  kBank phasorbnk kFrequency, 0, 2, iStart
  aBankK phasorbnk kFrequency, 0, 2, iStart
  aBankA phasorbnk aFrequency, 0, 2, iStart
  kAudioK downsamp aBankK
  kAudioA downsamp aBankA
  kSample init 0
  ; Sample zero is the initial phase. It reaches zero at sample four,
  ; so sample five must equal one step in both builds.
  if kSample == 5 then
    if kBank != iStep || kAudioK != iStep || kAudioA != iStep then
      printks "phasorbnk lost its phase: expected %.12g, got k %.12g, ak %.12g, aa %.12g\n", 0, iStep, kBank, kAudioK, kAudioA
      exitnowk -1
    endif
    gkChecks += 1
    turnoff
  endif
  kSample += 1
endin

instr CheckCompletion
  if i(gkChecks) != 5 then
    prints "All phasorbnk range and phase checks must complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Slow forward and reverse ramps reach phases that round to one in float.
i "CheckRange" 0 .01 .0009765625 .999999940395355224609375
i "CheckRange" 0 .01 -.0009765625 0
; Ordinary frequencies still wrap correctly in both directions.
i "CheckRange" 0 .01 440 .99
i "CheckRange" 0 .01 -440 .01
i "CheckPhaseIsKept" 0 .01
i "CheckCompletion" .02 .001
e
</CsScore>
</CsoundSynthesizer>
