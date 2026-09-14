<CsoundSynthesizer>
<CsOptions>
-i test_input_ramp_16.wav -n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
nchnls_i = 16
0dbfs = 256

instr 1
  ainput[] in
  kfirst max_k abs(ainput[0]), 1, 1
  klast max_k abs(ainput[15]), 1, 1
  if kfirst != 5 || klast != 80 then
    printks "in array read wrong input frames: %g, %g\n", 0, kfirst, klast
    exitnowk -1
  endif

  a1, a2, a3, a4, a5, a6, a7, a8, \
      a9, a10, a11, a12, a13, a14, a15, a16 inx
  kfirst max_k abs(a1), 1, 1
  klast max_k abs(a16), 1, 1
  if kfirst != 5 || klast != 80 then
    printks "inx read wrong input frames: %g, %g\n", 0, kfirst, klast
    exitnowk -1
  endif

  ag1, ag2, ag3 _in
  kfirst max_k abs(ag1), 1, 1
  klast max_k abs(ag3), 1, 1
  if kfirst != 5 || klast != 15 then
    printks "generic in read wrong input frames: %g, %g\n", 0, kfirst, klast
    exitnowk -1
  endif

  afirst, alast inch 1, 16
  kfirst max_k abs(afirst), 1, 1
  klast max_k abs(alast), 1, 1
  if kfirst != 5 || klast != 80 then
    printks "inch read wrong input frames: %g, %g\n", 0, kfirst, klast
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 .03125 .125
</CsScore>
</CsoundSynthesizer>
