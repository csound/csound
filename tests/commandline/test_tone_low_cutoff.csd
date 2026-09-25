<CsTest>
description = "tone-family coefficients retain low cutoffs at init and after cutoff changes"

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
gkChecks init 0

instr CheckFirstResponse
  iCutoff = p4
  iFeedIn = p5
  iRetune = p6
  kSample init 0
  kCutoff init (iRetune == 0 ? iCutoff : 100)
  ; For the retuning case, send silence before changing to the low cutoff.
  ; The first nonzero input then checks the new coefficient directly.
  kInput = 1
  if iRetune != 0 then
    kInput = (kSample == 0 ? 0 : 1)
    if kSample == 1 then
      kCutoff = iCutoff
    endif
  endif
  aInput = kInput
  aCutoff = kCutoff
  aTone tone aInput, kCutoff
  aToneAudio tone aInput, aCutoff
  aTonex tonex aInput, kCutoff, 1
  aTonexAudio tonex aInput, aCutoff, 1
  kTone tonek kInput, kCutoff
  aAtone atone aInput, kCutoff
  aAtoneAudio atone aInput, aCutoff
  aAtonex atonex aInput, kCutoff, 1
  aAtonexAudio atonex aInput, aCutoff, 1
  kAtone atonek kInput, kCutoff
  kRms rms aInput, iCutoff
  kAudioTone downsamp aTone
  kAudioModulated downsamp aToneAudio
  kTonex downsamp aTonex
  kTonexModulated downsamp aTonexAudio
  kAudioAtone downsamp aAtone
  kAtoneModulated downsamp aAtoneAudio
  kAtonex downsamp aAtonex
  kAtonexModulated downsamp aAtonexAudio

  if iRetune == 0 || kSample == 1 then
    ; A zero-state low-pass filter outputs c1 for a unit step's first sample.
    ; Its high-pass counterpart outputs c2=1-c1; RMS outputs sqrt(c1).
    kLowError = abs(kAudioTone-iFeedIn) + abs(kAudioModulated-iFeedIn)
    kLowError += abs(kTonex-iFeedIn) + abs(kTonexModulated-iFeedIn) + abs(kTone-iFeedIn)
    kHighError = abs(kAudioAtone-(1-iFeedIn)) + abs(kAtoneModulated-(1-iFeedIn))
    kHighError += abs(kAtonex-(1-iFeedIn)) + abs(kAtonexModulated-(1-iFeedIn))
    kHighError += abs(kAtone-(1-iFeedIn))
    iLowTolerance = max(iFeedIn*.00001, 1e-16)
    iRmsTolerance = max(sqrt(iFeedIn)*.00001, 1e-16)
    if !(kLowError < iLowTolerance && kHighError < .00001 && abs(kRms-sqrt(iFeedIn)) < iRmsTolerance) then
      printks "cutoff=%g retune=%g expected low=%g actual low=%g low error=%g high error=%g rms=%g\n", \
        0, iCutoff, iRetune, iFeedIn, kAudioTone, kLowError, kHighError, kRms
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
  kSample += 1
endin

instr CheckResults
  if i(gkChecks) != 7 then
    prints "tone completed %g coefficient checks; expected 7\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Reference c1 values from the manual's formula, evaluated at 70-digit precision.
;                                cutoff   expected c1               retune
; Ordinary and very low cutoffs must both pass a nonzero step response.
i "CheckFirstResponse" 0 [1/48000] 100      .013004483866065237567264   0
i "CheckFirstResponse" 0 [1/48000] .00001  1.30899693813901069e-9       0
; Preserve zero, quarter-rate, Nyquist, and negative-cutoff behavior.
i "CheckFirstResponse" 0 [1/48000] 0        0                         0
i "CheckFirstResponse" 0 [1/48000] 12000    .73205080756887729352745    0
i "CheckFirstResponse" 0 [1/48000] 24000    .82842712474619009760338    0
i "CheckFirstResponse" 0 [1/48000] -.00001 1.30899693813901069e-9       0
; Changing the cutoff must use the same stable coefficient as initialization.
i "CheckFirstResponse" 0 [2/48000] .00001  1.30899693813901069e-9       1
i "CheckResults" [3/48000] [1/48000]
e
</CsScore>
</CsoundSynthesizer>
