<CsTest>
description = "diskin2 keeps crossfading during a continuous kTranspose ramp"
[expect]
exit = 0
output = ["diskin2 crossfade ramp OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; With ksmps=64 and iwrap=128 the loop head spans more than one control period,
; so a speed change that reset the cached head every period would clear the
; capture before it can finish and leave the reader on the plain hard wrap.
; A continuous ramp from 1 to 1.1 changes kTranspose every period; the head must
; survive those steps so the crossfade reader keeps differing from the hard-wrap
; reader. Both the scalar and the array readers are checked.

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gkScalar init 0
gkArray  init 0

instr 1
  a = 0.5*sin(2*3.14159265*220*timeinsts()) + 0.3*sin(2*3.14159265*331*timeinsts())
  fout "diskin2_ramp_src.wav", 14, a
endin

instr 2
  ; ramp from 1 to about 1.08 over the note
  kPitch = 1 + 0.1*timeinsts()
  ;                        pitch  skip wrap  fmt win buf skip sync end
  aHard    diskin2 "diskin2_ramp_src.wav", kPitch, 0.10, 1,   0, 0, 0, 0, 0, 0.30
  aXf      diskin2 "diskin2_ramp_src.wav", kPitch, 0.10, 128, 0, 0, 0, 0, 0, 0.30
  aArrH[]  diskin2 "diskin2_ramp_src.wav", kPitch, 0.10, 1,   0, 0, 0, 0, 0, 0.30
  aArrX[]  diskin2 "diskin2_ramp_src.wav", kPitch, 0.10, 128, 0, 0, 0, 0, 0, 0.30

  kScalar  rms (aXf - aHard)
  kArray   rms (aArrX[0] - aArrH[0])
  gkScalar = (kScalar > gkScalar ? kScalar : gkScalar)
  gkArray  = (kArray  > gkArray  ? kArray  : gkArray)
endin

instr 3
  iScalar = i(gkScalar)
  iArray  = i(gkArray)
  if iScalar < 0.02 || iArray < 0.02 then
    prints "diskin2 crossfade ramp inactive: scalar=%g array=%g\n", iScalar, iArray
    exitnow(1)
  endif
  prints "diskin2 crossfade ramp OK\n"
endin
</CsInstruments>
<CsScore>
i1 0 1.0
i2 1.1 0.8
i3 2.0 0
e
</CsScore>
</CsoundSynthesizer>
