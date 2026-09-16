<CsTest>
description = "fin and fink reject mismatched channels and invalid skip counts"
[expect]
exit = "nonzero"
stderr = ["file channels do not match input arguments", "invalid frame skip"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
instr 1
  aSignal = .25
  fout "fin_channel_errors.wav", 14, aSignal, aSignal
endin
instr 2
  aSignal init 0
  fin "fin_channel_errors.wav", 0, 0, aSignal
endin
instr 3
  kSignal init 0
  fink "fin_channel_errors.wav", 0, 0, kSignal
endin
instr 4
  aSignal init 0
  fin "fin_channel_errors.wav", -1, 0, aSignal
endin
instr 5
  aSignal[] init 2
  fin "fin_channel_errors.wav", -1, 0, aSignal
endin
instr 6
  kSignal init 0
  fink "fin_channel_errors.wav", -1, 0, kSignal
endin
</CsInstruments>
<CsScore>
i1 0 .01
i2 .02 .01
i3 .02 .01
i4 .02 .01
i5 .02 .01
i6 .02 .01
</CsScore>
</CsoundSynthesizer>
