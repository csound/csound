<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
instr 1
  kInput[] fillarray 1, 2, 3
  kOutput[] unwrap kInput, p4
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 .5
i 1 0 .01 2
i 1 0 .01 1e20
e
</CsScore>
</CsoundSynthesizer>
