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
  iresult ftaudio 1, "test_ftaudio_i_range.wav", 16, 1, 3
  if iresult != 1 then
    exitnow(-1)
  endif

  ktrigger init 1
  kcycle init 0
  kresult ftaudio ktrigger, 1, "test_ftaudio_k_range.wav", 16, 0, 1, 3
  kasync ftaudio ktrigger, 1, "test_ftaudio_async.wav", 16, 1, 1, 3
  if kcycle == 0 && kasync != -1 then
    printks "async ftaudio did not report a pending write\n", 0
    exitnowk(-1)
  endif
  ktrigger = 0
  kcycle += 1
endin

instr 2
  iout1 ftgen 2, 0, 0, -1, "test_ftaudio_i_range.wav", 0, 0, 0
  iout2 ftgen 3, 0, 0, -1, "test_ftaudio_k_range.wav", 0, 0, 0
  indx = 0
  until indx == 4 do
    iexpected table indx + 2, 1
    ivalue1 table indx, iout1
    ivalue2 table indx, iout2
    if abs(ivalue1 - iexpected) > 0.000001 || \
       abs(ivalue2 - iexpected) > 0.000001 then
      prints "ftaudio wrote the wrong stereo frame range\n"
      exitnow(-1)
    endif
    indx += 1
  od
endin
</CsInstruments>
<CsScore>
f 1 0 0 -1 "./test_flooper_stereo_guard.wav" 0 0 0
i 1 0 0.01
i 2 0.02 0.01
e
</CsScore>
</CsoundSynthesizer>
