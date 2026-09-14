<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 8
nchnls = 2
0dbfs = 32768

instr 1
  aleft, aright flooper 1, 1, 0, 0, 0, 1
endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
