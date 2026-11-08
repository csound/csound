<CsTest>
description = "pvsfilter follows either input and publishes updates across source and filter reinit"
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
giAmplitudes ftgen 1, 0, -33, -7, 2, 33, 2
giFrequencies ftgen 2, 0, -33, -7, 0, 32, 2048

instr CheckFilter
  kCycle init 0
  kCycle += 1
  ; A fixed spectrum with amplitude 2 in every bin.
  fFixed pvsinit 64, p4, 64, 1
  pvsftr fFixed, giAmplitudes, giFrequencies
  kData[] init 66
  kBin = 0
  while kBin < 33 do
    kData[2*kBin] = kCycle/16 + kBin/64
    kData[2*kBin+1] = 128*kBin
    kBin += 1
  od
  if kCycle == 10 then
    reinit SOURCE
  endif
SOURCE:
  fLive pvsfromarray kData, p4, 64, 1
  rireturn

  if kCycle == 20 then
    reinit FILTERS
  endif
FILTERS:
  ; At full depth, swapping the inputs must give the same amplitudes.
  ; Each output must keep the frequencies of its own first input.
  fMaskUpdates pvsfilter fFixed, fLive, 1
  fInputUpdates pvsfilter fLive, fFixed, 1
  rireturn
  ; These consumers only copy a frame when its counter advances.
  fMaskCopy pvsfreeze fMaskUpdates, 0, 0
  fInputCopy pvsfreeze fInputUpdates, 0, 0
  kLive[] init 66
  kFixed[] init 66
  kMaskResult[] init 66
  kInputResult[] init 66
  kLiveFrame pvs2array kLive, fLive
  kFixedFrame pvs2array kFixed, fFixed
  kMaskFrame pvs2array kMaskResult, fMaskCopy
  kInputFrame pvs2array kInputResult, fInputCopy
  kPreviousLive init -1
  kPreviousMask init -1
  if kLiveFrame != kPreviousLive || kCycle == 20 then
    if kMaskFrame <= kPreviousMask then
      printks "pvsfilter did not publish an update at cycle %g\n", 0, kCycle
      exitnowk -1
    endif
  elseif kMaskFrame != kPreviousMask then
    printks "pvsfilter published a frame without a source update at cycle %g\n", 0, kCycle
    exitnowk -1
  endif
  kPreviousLive = kLiveFrame
  kPreviousMask = kMaskFrame
  kBin = 0
  while kBin < 33 do
    kExpected = kFixed[2*kBin]*kLive[2*kBin]
    if kMaskResult[2*kBin] != kExpected || kInputResult[2*kBin] != kExpected then
      printks "hop %g cycle %g bin %g: filter update=%g, input update=%g, expected=%g\n", 0, p4, kCycle, kBin, kMaskResult[2*kBin], kInputResult[2*kBin], kExpected
      exitnowk -1
    endif
    if kMaskResult[2*kBin+1] != kFixed[2*kBin+1] || kInputResult[2*kBin+1] != kLive[2*kBin+1] then
      printks "pvsfilter changed a frequency at cycle %g bin %g\n", 0, kCycle, kBin
      exitnowk -1
    endif
    kBin += 1
  od
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 180 then
    prints "Not all pvsfilter frame checks finished\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Thirty blocks per note. Larger hops also check blocks without a new frame.
i "CheckFilter" 0 .05859375 16
i "CheckFilter" 0 .05859375 32
i "CheckFilter" 0 .05859375 40
; Reuse note instances after their sources and outputs have advanced.
i "CheckFilter" .125 .05859375 16
i "CheckFilter" .125 .05859375 32
i "CheckFilter" .125 .05859375 40
i "CheckCompletion" .25 .01
e
</CsScore>
</CsoundSynthesizer>
