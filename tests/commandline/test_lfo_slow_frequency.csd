<CsTest>
description = "lfo keeps advancing at slow positive frequencies"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
gkFailures init 0

instr CheckSlowSaw
  iFrequency = p4
  aSaw lfo 1, iFrequency, 4
  kSaw lfo 1, iFrequency, 4
  kLastAudioSample vaget ksmps-1, aSaw
  kBlock init 0
  ; Both oscillators start at phase zero. A saw's value equals its phase.
  kExpectedControl = kBlock*ksmps*iFrequency/sr
  kExpectedAudio = (kBlock*ksmps + ksmps-1)*iFrequency/sr
  if !(abs(kSaw-kExpectedControl) < 1e-12 && abs(kLastAudioSample-kExpectedAudio) < 1e-12) then
    if kBlock == 0 then
      printks "lfo frequency=%g expected last audio sample=%g actual=%g\n", \
        0, iFrequency, kExpectedAudio, kLastAudioSample
    endif
    gkFailures += 1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr CheckResults
  if i(gkChecks) != 3 || i(gkFailures) != 0 then
    prints "lfo checked %g frequencies, with %g failing blocks\n", i(gkChecks), i(gkFailures)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; At 1/512 Hz the existing 24-bit phase increment is exactly one.
i "CheckSlowSaw" 0 [64/32768] [1/512]
; At 1/1024 Hz it should retain half a phase step instead of stopping.
i "CheckSlowSaw" 0 [64/32768] [1/1024]
; Slow enough to expose truncation at control rate too.
i "CheckSlowSaw" 0 [64/32768] [1/65536]
i "CheckResults" [80/32768] [16/32768]
e
</CsScore>
</CsoundSynthesizer>
