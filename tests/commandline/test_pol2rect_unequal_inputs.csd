<CsTest>
description = "pol2rect rejects input lengths that diverge during performance"

[expect]
exit = "nonzero"
stderr = ["pol2rect: expected equal-length one-dimensional magnitude and phase arrays"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
instr 1
  kMags[] init 5
  kPhases[] init 5
  kCycle timeinstk
  if kCycle == 2 then
    trim kMags, 3
  endif
  kOut[] pol2rect kMags, kPhases
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
