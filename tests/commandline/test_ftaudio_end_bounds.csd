<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  kresult ftaudio 1, 1, "test_ftaudio_invalid.wav", 16, 0, 0, 9
endin
</CsInstruments>
<CsScore>
f 1 0 0 -1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
