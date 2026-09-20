<CsTest>
description = "fout rejects array growth and incompatible writers sharing a file"
[expect]
exit = "nonzero"
stderr = ["fout: array channel count changed", "fout: file channel count does not match input"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 512
nchnls = 1
0dbfs = 1

instr 1
  aChannels[] init 2
  trim_i aChannels, 1
  fout "fout_growing.wav", 14, aChannels
  ; The next cycle must not write two channels into a one-channel buffer.
  trim aChannels, 2
endin

instr 2
  aSignal init 0
  fout "fout_shared.wav", 14, aSignal, aSignal
endin
instr 3
  aSignal init 0
  fout "fout_shared.wav", 14, aSignal
endin
instr 4
  aChannels[] init 1
  fout "fout_shared.wav", 14, aChannels
endin
instr 5
  foutk "fout_shared.wav", 1, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
i 3 .2 .1
i 4 .2 .1
i 5 .2 .1
e
</CsScore>
</CsoundSynthesizer>
