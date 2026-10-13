<CsTest>
description = "diskin2 keeps crossfading during a kTranspose ramp in rt async mode"
args = []

[expect]
exit = 0
output = ["diskin2 async crossfade ramp OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime -d -m128
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

; The source is a slow ramp with a step at the loop boundary, so a reader that
; falls back to the plain hard wrap shows a large adjacent-sample jump there.
; The crossfade head is much longer than one control period, so the async
; readers only keep it if a worker poll cannot clear the ramp state; the first
; loop passes are gated out because the head is still being captured then.
; Both the scalar and array async readers are checked against the synchronous
; reader.
; Write the fixture before playback. Realtime fout can drop samples when its
; writer falls behind, putting unrelated jumps into every reader's input.
giSource ftgen 0, 0, -17640, -7, -0.5, 17640, -0.1
giSaved ftaudio giSource, "async-ramp-source.wav", 14
gkFixed init 0
gkAsync init 0
gkArray init 0
gkSync init 0
gkHard init 0

instr 2
  kPitch = 1 + 0.1*timeinsts()
  ;                      pitch   skip wrap fmt win buf  skip sync
  aFixed diskin2 "async-ramp-source.wav", 1,      0, 4096, 0, 2, 1024, 0, 0
  aAsync diskin2 "async-ramp-source.wav", kPitch, 0, 4096, 0, 2, 1024, 0, 0
  aArray[] diskin2 "async-ramp-source.wav", kPitch, 0, 4096, 0, 2, 1024, 0, 0
  aSync  diskin2 "async-ramp-source.wav", kPitch, 0, 4096, 0, 2, 1024, 0, 1

  aHard  diskin2 "async-ramp-source.wav", kPitch, 0, 1, 0, 2, 1024, 0, 1

  ; measure adjacent-sample jumps, ignoring startup and the head capture
  kGate = (timeinsts() > 1.2 ? 1 : 0)
  aD1 = (aFixed - delay1(aFixed)) * kGate
  aD2 = (aAsync - delay1(aAsync)) * kGate
  aD3 = (aArray[0] - delay1(aArray[0])) * kGate
  aD4 = (aSync - delay1(aSync)) * kGate
  aD5 = (aHard - delay1(aHard)) * kGate
  ; Measure in memory so the result also avoids async recording.
  gkFixed peak aD1
  gkAsync peak aD2
  gkArray peak aD3
  gkSync peak aD4
  gkHard peak aD5
  out aAsync
endin

instr 3
  iFixed = i(gkFixed)
  iAsync = i(gkAsync)
  iArray = i(gkArray)
  iSync = i(gkSync)
  iHard = i(gkHard)
  prints "diskin2 async ramp jumps: fixed=%g async=%g array=%g sync=%g hard=%g\n", \
    iFixed, iAsync, iArray, iSync, iHard
  ; omit the marker on failure; the runner fails the test on the missing
  ; output.
  ; The plain loop must still produce a jump, proving the measurement reaches
  ; the loop boundary and would catch a lost crossfade.
  if iHard > 0.2 && iFixed <= 0.05 && iSync <= 0.05 && iAsync <= 0.05 && iArray <= 0.05 then
    prints "diskin2 async crossfade ramp OK\n"
  else
    prints "diskin2 async crossfade ramp fell back to hard wrap\n"
  endif
endin
</CsInstruments>
<CsScore>
i2 0.5 2.0
i3 2.6 0.1
</CsScore>
</CsoundSynthesizer>
