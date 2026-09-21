<CsTest>
description = "diskin2 crossfade re-captures the loop head when kTranspose changes"
[expect]
exit = 0
output = ["diskin2 crossfade speed change OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; The loop is 0.50..0.54 s (1764 frames) with iwrap=100. kTranspose is 1 for
; the first 0.3 s, so the head is first captured over 100 source frames
; (span 100). When kTranspose becomes 2 the cap allows span 200, so the head
; must be refreshed: the steady-state loop period is (1764-200)/2 = 782 output
; frames. If the stale speed-1 head were kept, the period would stay
; (1764-100)/2 = 832 frames.
;
; Autocorrelation at the two candidate periods separates the cases: the signal
; must match a 782-frame delayed copy and must not match an 832-frame one.

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  kPitch = (timeinsts() < 0.3 ? 1 : 2)
  aXf    diskin2 "fox.wav", kPitch, 0.50, 100, 0, 2, 0, 0, 0, 0.54
  aArr[] diskin2 "fox.wav", kPitch, 0.50, 100, 0, 2, 0, 0, 0, 0.54

  ; ignore the transition and the first delay-line contents
  kGate = (timeinsts() > 0.6 ? 1 : 0)
  a782s = (aXf - delay(aXf, 782/sr)) * kGate
  a832s = (aXf - delay(aXf, 832/sr)) * kGate
  a782a = (aArr[0] - delay(aArr[0], 782/sr)) * kGate
  a832a = (aArr[0] - delay(aArr[0], 832/sr)) * kGate

  fout "diskin2_speed_change_782s.wav", 14, a782s
  fout "diskin2_speed_change_832s.wav", 14, a832s
  fout "diskin2_speed_change_782a.wav", 14, a782a
  fout "diskin2_speed_change_832a.wav", 14, a832a
endin

instr 2
  i782s = filepeak("diskin2_speed_change_782s.wav", 1)
  i832s = filepeak("diskin2_speed_change_832s.wav", 1)
  i782a = filepeak("diskin2_speed_change_782a.wav", 1)
  i832a = filepeak("diskin2_speed_change_832a.wav", 1)

  if i782s > 0.05 || i782a > 0.05 then
    prints "diskin2 speed change kept stale head: 782=%g arr=%g\n", i782s, i782a
    exitnow(1)
  endif
  if i832s < 0.05 || i832a < 0.05 then
    prints "diskin2 speed change did not refresh: 832=%g arr=%g\n", i832s, i832a
    exitnow(1)
  endif
  prints "diskin2 crossfade speed change OK\n"
endin
</CsInstruments>
<CsScore>
i1 0 0.9
i2 0.95 0
e
</CsScore>
</CsoundSynthesizer>
