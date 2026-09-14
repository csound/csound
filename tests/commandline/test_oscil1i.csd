<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  ; p4: table, p5: duration, p6: delay, p7/p8: expected start/step.
  iEnd = ftlen(p4)
  iDelay = int(p6 * kr)
  kCount init 0
  kAmp = 1 + kCount / 8
  kActual oscil1i p6, kAmp, p5, p4
  kPosition limit p7 + max(kCount - iDelay, 0) * p8, 0, iEnd
  kExpected = kPosition * kAmp
  if !(abs(kActual - kExpected) <= 0.0001) then
    printks "oscil1i mismatch: table=%d duration=%.9g sample=%d actual=%.9g expected=%.9g\n", 0, p4, p5, kCount, kActual, kExpected
    exitnowk(-1)
  endif
  kCount += 1
endin

instr 2
  ; oscil1 shares the initializer: zero duration must also hold its endpoint.
  kCount init 0
  kAmp = 1 + kCount / 8
  kActual oscil1 p5, kAmp, 0, p4
  kExpected = ftlen(p4) * kAmp
  if !(abs(kActual - kExpected) <= 0.0001) then
    printks "oscil1 zero-duration mismatch: actual=%.9g expected=%.9g\n", 0, kActual, kExpected
    exitnowk(-1)
  endif
  kCount += 1
endin
</CsInstruments>
<CsScore>
; Linear tables with explicit guard values distinct from the last real sample.
f1 0 9 -2 0 1 2 3 4 5 6 7 8
f2 0 10 -2 0 1 2 3 4 5 6 7 8 9 10
; Zero duration, with and without a delay.
i1 0 .004 1 0 0 8 0
i1 0 .004 2 0 0 10 0
i1 0 .004 1 0 .000244140625 8 0
i1 0 .004 2 0 .000244140625 10 0
; Eight control periods, with and without a two-period delay.
i1 0 .004 1 .0009765625 0 0 1
i1 0 .004 2 .0009765625 0 0 1.25
i1 0 .004 1 .0009765625 .000244140625 0 1
i1 0 .004 2 .0009765625 .000244140625 0 1.25
; Reverse scans retain their existing starting position.
i1 0 .004 1 -.0009765625 0 8 -1
i1 0 .004 2 -.0009765625 0 9 -1.25
i1 0 .004 1 -.0009765625 .000244140625 8 -1
i1 0 .004 2 -.0009765625 .000244140625 9 -1.25
; Scans shorter than one control period finish on the next sample.
i1 0 .004 1 .00006103515625 0 0 16
i1 0 .004 2 .00006103515625 0 0 20
i1 0 .004 1 -.00006103515625 0 8 -16
i1 0 .004 2 -.00006103515625 0 9 -20
i1 0 .004 1 1e-20 0 0 16
i1 0 .004 2 1e-20 0 0 20
i1 0 .004 1 -1e-20 0 8 -16
i1 0 .004 2 -1e-20 0 9 -20
i2 0 .004 1 0
i2 0 .004 2 0
i2 0 .004 1 .000244140625
i2 0 .004 2 .000244140625
</CsScore>
</CsoundSynthesizer>
