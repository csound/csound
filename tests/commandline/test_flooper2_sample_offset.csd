<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 16
nchnls = 2
0dbfs = 32768

instr 1
  aleft, aright flooper2 1, 1, 0, 6.1 / sr, 2.1 / sr, 1, 0, 2
  kright max_k abs(aright), 1, 1
  if abs(kright - (21 / 27)) > .001 then
    printks "flooper2 right startup peak: %g\n", 0, kright
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 .0001133787 .0000453515
</CsScore>
</CsoundSynthesizer>
