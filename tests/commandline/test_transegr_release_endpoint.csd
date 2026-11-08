<CsTest>
description = "transegr outputs and holds its release endpoint"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 4
nchnls = 1
0dbfs = 1
gkLastControl init -1
gkLastAudio init -1
gkReleasePeriods init 0

instr Envelope
  ; p4: release duration in control periods; p5: extra note lifetime.
  ; p6: curve type; p7: use only a release segment.
  xtratim p5/kr
  gkReleasePeriods = 0
  if p7 == 0 then
    kEnvelope transegr 1, 4/kr, 0, 2, p4/kr, p6, 0
    aEnvelope transegr 1, 4/kr, 0, 2, p4/kr, p6, 0
    iReleaseStart = 1 + min(p3*kr/4, 1)
  else
    kEnvelope transegr 2, p4/kr, p6, 0
    aEnvelope transegr 2, p4/kr, p6, 0
    iReleaseStart = 2
  endif
  gkLastControl = kEnvelope
  gkLastAudio vaget ksmps-1, aEnvelope

  kReleasing release
  kPeriod init 0
  if kReleasing == 1 then
    ; Control durations truncate to whole periods; audio durations round to samples.
    iControlSteps = int(p4)
    iAudioSteps = int(p4*ksmps + .5)
    kPosition = 1
    if iControlSteps > 0 then
      kPosition = min(kPeriod/iControlSteps, 1)
    endif
    kShape = kPosition
    if p6 != 0 then
      kShape = (1-exp(p6*kPosition))/(1-exp(p6))
    endif
    kExpected = iReleaseStart*(1-kShape)
    if !(abs(kEnvelope-kExpected) < .00001) then
      printks "Release period %g: expected control %g, got %g\n", 0, kPeriod, kExpected, kEnvelope
      exitnowk -1
    endif

    kSample = 0
    while kSample < ksmps do
      kPosition = 1
      if iAudioSteps > 0 then
        kPosition = min((kPeriod*ksmps+kSample)/iAudioSteps, 1)
      endif
      kShape = kPosition
      if p6 != 0 then
        kShape = (1-exp(p6*kPosition))/(1-exp(p6))
      endif
      kExpected = iReleaseStart*(1-kShape)
      kActual vaget kSample, aEnvelope
      if !(abs(kActual-kExpected) < .00001) then
        printks "Release period %g, sample %g: expected audio %g, got %g\n", 0, kPeriod, kSample, kExpected, kActual
        exitnowk -1
      endif
      kSample += 1
    od
    kPeriod += 1
    gkReleasePeriods = kPeriod
  endif
endin

instr CheckEndpoint
  if !(abs(i(gkLastControl)) < .00001 && abs(i(gkLastAudio)) < .00001) then
    prints "Expected endpoint 0: last control=%g, last audio=%g\n", i(gkLastControl), i(gkLastAudio)
    exitnow -1
  endif
  if i(gkReleasePeriods) != p4 then
    prints "Expected %g release periods, got %g\n", p4, i(gkReleasePeriods)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Four-period release needs a fifth period to output the endpoint.
i "Envelope" 0 [8/256] 4 0 0 0
i "CheckEndpoint" .1 .004 5
; Zero duration jumps immediately; a fractional duration keeps sample timing.
i "Envelope" .2 [8/256] 0 0 0 0
i "CheckEndpoint" .3 .004 1
i "Envelope" .4 [8/256] .6 0 0 0
i "CheckEndpoint" .5 .004 1
i "Envelope" .6 [8/256] 1.6 0 2 0
i "CheckEndpoint" .7 .004 2
; A shared tail must neither omit the endpoint nor stretch the curve.
i "Envelope" .8 [8/256] 4 4 -2 0
i "CheckEndpoint" .9 .004 5
i "Envelope" 1 [8/256] 4 8 2 0
i "CheckEndpoint" 1.1 .004 8
; Note-off halfway through the rise releases from its current value.
i "Envelope" 1.2 [2/256] 4 0 -2 0
i "CheckEndpoint" 1.3 .004 5
; A release-only envelope holds its start value until note-off.
i "Envelope" 1.4 [8/256] 1.6 0 0 1
i "CheckEndpoint" 1.5 .004 2
e
</CsScore>
</CsoundSynthesizer>
