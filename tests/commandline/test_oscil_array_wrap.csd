<CsTest>
description = "Array oscillators match guarded tables for single, odd and power-of-two lengths"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

instr 1
  iWave[] genarray 1, p4
  ; Exact length with a wraparound guard, including lengths one and five.
  iTable ftgen 0, 0, -p4-.25, -7, 1, p4, p4+1
  kArray oscil 1, 0, iWave, .875
  kTable oscil 1, 0, iTable, .875
  kArrayI oscili 1, 0, iWave, .875
  kTableI oscili 1, 0, iTable, .875
  kArray3 oscil3 1, 0, iWave, .875
  kTable3 oscil3 1, 0, iTable, .875
  aArray oscil 1, 0, iWave, .875
  aArrayI oscili 1, 0, iWave, .875
  aArray3 oscil3 1, 0, iWave, .875
  ; Growing the source must not invalidate the initialized waveform.
  trim_i iWave, p4+8
  kAudio downsamp aArray
  kAudioI downsamp aArrayI
  kAudio3 downsamp aArray3
  if !(abs(kArray-kTable) < .00001) || !(abs(kArrayI-kTableI) < .00001) || !(abs(kArray3-kTable3) < .00001) || !(abs(kAudio-kTable) < .00001) || !(abs(kAudioI-kTableI) < .00001) || !(abs(kAudio3-kTable3) < .00001) then
    printks "Array oscillator wrap mismatch, length %g\n", 0, p4
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1
i 1 .02 .01 3
i 1 .04 .01 4
i 1 .06 .01 5
e
</CsScore>
</CsoundSynthesizer>
