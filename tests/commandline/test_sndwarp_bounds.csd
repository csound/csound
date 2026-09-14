<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkchecks init 0

; Check windowed and unwindowed mono/stereo outputs at both input rates.
instr 1, 2
  kcount init 0
  if p1 == 1 then
    ktime init p5
    kspeed init p4
    amono, aclean sndwarp 1, ktime, kspeed, 1, p7/sr, 4, 0, 1, 3, p6
    aleft, aright, acleft, acright sndwarpst 1, ktime, kspeed, 2, p7/sr, 4, 0, 1, 3, p6
  else
    atime = p5
    aspeed = p4
    amono, aclean sndwarp 1, atime, aspeed, 1, p7/sr, 4, 0, 1, 3, p6
    aleft, aright, acleft, acright sndwarpst 1, atime, aspeed, 2, p7/sr, 4, 0, 1, 3, p6
  endif
  kmono downsamp amono
  kclean downsamp aclean
  kleft downsamp aleft
  kright downsamp aright
  kcleft downsamp acleft
  kcright downsamp acright
  kexpected table kcount, p8
  ; The right channel holds the left channel's values plus 100.
  if !(abs(kmono - kexpected) < 0.001 && abs(kclean - kexpected) < 0.001 && \
       abs(kleft - kexpected) < 0.001 && abs(kright - kexpected - 100) < 0.001 && \
       abs(kcleft - kexpected) < 0.001 && abs(kcright - kexpected - 100) < 0.001) then
    printks "sndwarp mismatch: rate=%d case=%d sample=%d expected=%.3f mono=%.3f clean=%.3f left=%.3f right=%.3f cleanleft=%.3f cleanright=%.3f\n", 0, p1, p8, kcount, kexpected, kmono, kclean, kleft, kright, kcleft, kcright
    exitnowk(-1)
  endif
  gkchecks += 1
  kcount += 1
  if kcount == 8 then
    turnoff
  endif
endin

instr 99
  if i(gkchecks) != 144 then
    prints "sndwarp: not all sample checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 8 -2 10 11 12 13 14 15 16 17
f 2 0 16 -2 10 110 11 111 12 112 13 113 14 114 15 115 16 116 17 117
f 3 0 4 -2 1 1 1 1

; Expected frames span two windows, including each offset update.
f 10 0 8 -2 11 10 10 10 11 10 10 10
f 11 0 8 -2 11 10.5 10 10 11 10.5 10 10
f 12 0 8 -2 10 11 12 13 10 10 10 11
f 13 0 8 -2 12 13 14 15 10 10 10 11
f 14 0 8 -2 12 13 14 15 12 13 14 15
f 15 0 8 -2 16 17 17 17 16 17 17 17
f 16 0 8 -2 11 12 13 14 13 14 15 16
f 17 0 8 -2 10 10 11 12 10 10 11 12

;       speed   time       mode begin expected
; Reverse playback crossing the first frame, including large steps.
i 1 0 0.001 -1    0          1    1     10
i 2 0 0.001 -1    0          1    1     10
i 1 0 0.001 -0.5  0          1    1     11
i 2 0 0.001 -0.5  0          1    1     11
i 1 0 0.001 -100  0          1    1     10
i 2 0 0.001 -100  0          1    1     10
; A negative time pointer, negative time scale, and zero time scale.
i 1 0 0.001  1   -0.00025    1    0     12
i 2 0 0.001  1   -0.00025    1    0     12
i 1 0 0.001  1   -1          0    2     13
i 2 0 0.001  1   -1          0    2     13
i 1 0 0.001  1    0          0    2     14
i 2 0 0.001  1    0          0    2     14
; Preserve the last-frame hold and ordinary forward time scaling.
i 1 0 0.001  1    0          1    6     15
i 2 0 0.001  1    0          1    6     15
i 1 0 0.001  1    2          0    1     16
i 2 0 0.001  1    2          0    1     16
; A starting offset before the first frame.
i 1 0 0.001  1    0          1   -1     17
i 2 0 0.001  1    0          1   -1     17
i 99 0.002 0.001
</CsScore>
</CsoundSynthesizer>
