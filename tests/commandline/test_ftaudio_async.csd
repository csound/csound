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
  ktrigger init 1
  kcycle init 0
  kasync ftaudio ktrigger, 1, "test_ftaudio_async.wav", 16, 1, 1, 3
  if kcycle == 0 && kasync != -1 then
    printks "async ftaudio did not report a pending write\n", 0
    exitnowk(-1)
  endif
  ktrigger = 0
  kcycle += 1
endin
</CsInstruments>
<CsScore>
f 1 0 0 -1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
