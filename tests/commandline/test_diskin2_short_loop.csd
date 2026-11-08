<CsTest>
description = "diskin2 bounded loops keep reads inside the loop for short loops"
[expect]
exit = 0
output = ["diskin2 short loop wrap OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; A short bounded loop is read from a generated soundfile whose contents are
; 1.0 everywhere except the loop region [256, 264), which is 0.0. A read that
; escapes the loop therefore shows up as output far above zero.
;
; The failing cases are wide sinc interpolation (window much longer than the
; loop) and fast playback (increment longer than the loop); both need the read
; position wrapped by more than one loop length. Scalar and array outputs, the
; crossfade and hard-wrap readers, and a one-frame loop are all checked.

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  kCount init 0
  aVal = (kCount < 256 || kCount >= 264) ? 1.0 : 0.0
  fout "diskin2_short_loop_src.wav", 14, aVal
  kCount += 1
endin

instr 2
  ;                pitch  skip       wrap     fmt win  buf skip sync end
  aXf     diskin2 "diskin2_short_loop_src.wav", 1, 256.5/sr, 1000000, 0, 64, 0, 0, 0, 264/sr
  aXfArr[] diskin2 "diskin2_short_loop_src.wav", 1, 256.5/sr, 1000000, 0, 64, 0, 0, 0, 264/sr
  aHard   diskin2 "diskin2_short_loop_src.wav", 1, 256.5/sr, 1, 0, 64, 0, 0, 0, 264/sr
  aFastXf diskin2 "diskin2_short_loop_src.wav", 20, 256.5/sr, 1000000, 0, 2, 0, 0, 0, 264/sr
  aFastHd diskin2 "diskin2_short_loop_src.wav", 20, 256.5/sr, 1, 0, 2, 0, 0, 0, 264/sr
  aLen1   diskin2 "diskin2_short_loop_src.wav", 1, 256.5/sr, 1, 0, 64, 0, 0, 0, 257/sr
  aRef    diskin2 "diskin2_short_loop_src.wav", 1
  fout "diskin2_short_loop_xf.wav", 14, aXf
  fout "diskin2_short_loop_arr.wav", 14, aXfArr[0]
  fout "diskin2_short_loop_hard.wav", 14, aHard
  fout "diskin2_short_loop_fastxf.wav", 14, aFastXf
  fout "diskin2_short_loop_fasthd.wav", 14, aFastHd
  fout "diskin2_short_loop_len1.wav", 14, aLen1
  fout "diskin2_short_loop_ref.wav", 14, aRef
endin

instr 3
  iXf     filepeak "diskin2_short_loop_xf.wav", 1
  iArr    filepeak "diskin2_short_loop_arr.wav", 1
  iHard   filepeak "diskin2_short_loop_hard.wav", 1
  iFastXf filepeak "diskin2_short_loop_fastxf.wav", 1
  iFastHd filepeak "diskin2_short_loop_fasthd.wav", 1
  iLen1   filepeak "diskin2_short_loop_len1.wav", 1
  iRef    filepeak "diskin2_short_loop_ref.wav", 1
  if iRef < 0.9 then
    prints "diskin2 short loop source is silent: ref=%g\n", iRef
    exitnow(1)
  endif
  if iXf > 0.001 || iArr > 0.001 || iHard > 0.001 || iFastXf > 0.001 || iFastHd > 0.001 || iLen1 > 0.001 then
    prints "diskin2 short loop read outside loop: xf=%g arr=%g hard=%g fastxf=%g fasthd=%g len1=%g\n", iXf, iArr, iHard, iFastXf, iFastHd, iLen1
    exitnow(1)
  endif
  prints "diskin2 short loop wrap OK\n"
endin
</CsInstruments>
<CsScore>
i1 0 0.02
i2 0.03 0.02
i3 0.06 0
e
</CsScore>
</CsoundSynthesizer>
