<CsTest>
description = "resonx matches a reson cascade when its output reuses audio inputs"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckAudioControls
  iLayers = p4
  iScaling = p5
  aPhase phasor 37
  aInput = .001 + .002*aPhase
  aCutoff = 400 + 1200*aPhase
  aBandwidth = 100 + 300*aPhase

  ; A cascade of separate reson calls defines the expected output.
  if iLayers == 1 then
    aExpected reson aInput, aCutoff, aBandwidth, iScaling
  else
    aFirst reson aInput, aCutoff, aBandwidth, iScaling
    aSecond reson aFirst, aCutoff, aBandwidth, iScaling
    aExpected reson aSecond, aCutoff, aBandwidth, iScaling
  endif

  aSeparate resonx aInput, aCutoff, aBandwidth, iLayers, iScaling
  aReusedInput = aInput
  aReusedInput resonx aReusedInput, aCutoff, aBandwidth, iLayers, iScaling
  aReusedCutoff = aCutoff
  aReusedCutoff resonx aInput, aReusedCutoff, aBandwidth, iLayers, iScaling
  aReusedBandwidth = aBandwidth
  aReusedBandwidth resonx aInput, aCutoff, aReusedBandwidth, iLayers, iScaling

  ; Compare every active sample, including partial start and end blocks.
  aError = abs(aSeparate-aExpected) + abs(aReusedInput-aExpected)
  aError += abs(aReusedCutoff-aExpected) + abs(aReusedBandwidth-aExpected)
  aError /= 1+abs(aExpected)
  kError max_k aError, 1, 1
  if !(kError < .000001) then
    printks "resonx input reuse: layers=%g, scaling=%g, error=%g\n", 0, iLayers, iScaling, kError
    exitnowk -1
  endif
  if timeinstk() == 1 then
    gkChecks += 1
  endif
endin

instr CheckMixedRates
  iScaling = p4
  aPhase phasor 53
  aInput = .001 + .002*aPhase
  aCutoff = 400 + 1200*aPhase
  aBandwidth = 100 + 300*aPhase
  kCutoff = 900 + 50*timeinsts()
  kBandwidth = 150 + 20*timeinsts()

  ; Only the cutoff is audio-rate, and its variable becomes the output.
  aFirst reson aInput, aCutoff, kBandwidth, iScaling
  aExpectedCutoff reson aFirst, aCutoff, kBandwidth, iScaling
  aCutoff resonx aInput, aCutoff, kBandwidth, 2, iScaling

  ; Only the bandwidth is audio-rate, and its variable becomes the output.
  aFirstBandwidth reson aInput, kCutoff, aBandwidth, iScaling
  aExpectedBandwidth reson aFirstBandwidth, kCutoff, aBandwidth, iScaling
  aBandwidth resonx aInput, kCutoff, aBandwidth, 2, iScaling

  ; Control-rate parameters must still match the same cascade.
  aFirstControl reson aInput, kCutoff, kBandwidth, iScaling
  aExpectedControl reson aFirstControl, kCutoff, kBandwidth, iScaling
  aControl resonx aInput, kCutoff, kBandwidth, 2, iScaling

  aError = abs(aCutoff-aExpectedCutoff)/(1+abs(aExpectedCutoff))
  aError += abs(aBandwidth-aExpectedBandwidth)/(1+abs(aExpectedBandwidth))
  aError += abs(aControl-aExpectedControl)/(1+abs(aExpectedControl))
  kError max_k aError, 1, 1
  if !(kError < .000001) then
    printks "resonx mixed-rate control reuse: scaling=%g, error=%g\n", 0, iScaling, kError
    exitnowk -1
  endif
  if timeinstk() == 1 then
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 9 then
    prints "All nine resonx comparisons must run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; One and three layers, with all scaling modes and partial start/end blocks.
i "CheckAudioControls" 0 .030375 1 0
i "CheckAudioControls" .040125 .030375 1 1
i "CheckAudioControls" .080625 .030375 1 2
i "CheckAudioControls" .120875 .030375 3 0
i "CheckAudioControls" .160375 .030375 3 1
i "CheckAudioControls" .200625 .030375 3 2
; Two layers with each mix of audio- and control-rate parameters.
i "CheckMixedRates" .240125 .030375 0
i "CheckMixedRates" .280625 .030375 1
i "CheckMixedRates" .320875 .030375 2
i "CheckCompletion" .36 .002
e
</CsScore>
</CsoundSynthesizer>
