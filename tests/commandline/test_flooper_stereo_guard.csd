<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 32768

instr 1
  aleft, aright flooper 1, 0.5, 0, 0.000136, 0, 1
  kright downsamp aright
  ksample init 0
  if ksample == 19 then
    ; Frame 4 and the wrapped frame 0 should be interpolated here.
    kexpected = 22 / 27
    if abs(kright - kexpected) > 0.01 then
      printks "flooper stereo wrap mismatch: expected %.6f, got %.6f\\n", 0, kexpected, kright
      exitnowk(1)
    endif
    turnoff
  endif
  ksample += 1
endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 0 0.001
e
</CsScore>
</CsoundSynthesizer>
