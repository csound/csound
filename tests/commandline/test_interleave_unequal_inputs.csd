<CsTest>
description = "interleave rejects input lengths that diverge during performance"

[expect]
exit = "nonzero"
stderr = ["interleave: expected equal-length one-dimensional inputs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
instr 1
  kA[] fillarray 1, 2, 3, 4
  kB[] fillarray 5, 6, 7, 8
  kCycle timeinstk
  if kCycle == 2 then
    trim kA, 2
  endif
  kOut[] interleave kA, kB
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
