<CsTest>
description = "trigphasor resets at the interpolated crossing using the previous rate"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0
gkFailures init 0

instr CheckReset
  iTriggerBefore = p4
  iTriggerAfter = p5
  iRateBefore = p6
  iRateAfter = p7
  iTriggerSample = p8
  iExpectedPhase = p9

  kSample init 0
  aTrigger = kSample < iTriggerSample ? iTriggerBefore : iTriggerAfter
  kRate = kSample < iTriggerSample ? iRateBefore : iRateAfter
  aRate = kRate

  ; Both calls have an audio trigger. Only the rate argument differs.
  aControlRatePhase trigphasor aTrigger, kRate, 0, 16, 4
  aAudioRatePhase trigphasor aTrigger, aRate, 0, 16, 4
  kControlRatePhase downsamp aControlRatePhase
  kAudioRatePhase downsamp aAudioRatePhase

  if kSample == iTriggerSample then
    if abs(kControlRatePhase-iExpectedPhase) + \
       abs(kAudioRatePhase-iExpectedPhase) > .000001 then
      printks "trigphasor: trigger=%g->%g rate=%g->%g sample=%g expected=%g control-rate result=%g audio-rate result=%g\n", \
        0, iTriggerBefore, iTriggerAfter, \
        iRateBefore, iRateAfter, iTriggerSample, \
        iExpectedPhase, kControlRatePhase, kAudioRatePhase
      gkFailures += 1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin

instr CheckCoverage
  if i(gkChecks) != 8 || i(gkFailures) != 0 then
    prints "trigphasor reset failures=%g completed=%g (expected 8)\n", i(gkFailures), i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Each note lasts eight samples. Every reset starts from phase 4.
;                                    trigger    rate       trigger expected
;                                    from/to    old/new    sample  phase

; Symmetric crossing: half of the previous .25 step has elapsed: 4 + .125.
i "CheckReset" 0 [8/8192]              -1  1     .25  .25     2     4.125
; One quarter of the .25 step follows the crossing: 4 + .0625.
i "CheckReset" 0 [8/8192]              -3  1     .25  .25     2     4.0625
; The previous trigger was exactly zero: one full step has elapsed.
i "CheckReset" 0 [8/8192]               0  1     .25  .25     2     4.25

; A rate change at the trigger must not change the previous interval.
i "CheckReset" 0 [8/8192]              -1  1     .25  .75     2     4.125
; This also holds when the old rate was negative or zero.
i "CheckReset" 0 [8/8192]              -1  1    -.25  .75     2     3.875
i "CheckReset" 0 [8/8192]              -1  1      0   .75     2     4

; At note start there is no previous interval: reset exactly to phase 4.
i "CheckReset" 0 [8/8192]              -1  1     .25  .25     0     4
i "CheckReset" 0 [8/8192]              -1  1    -.25 -.25     0     4

i "CheckCoverage" [16/8192] [1/8192]
e
</CsScore>
</CsoundSynthesizer>
