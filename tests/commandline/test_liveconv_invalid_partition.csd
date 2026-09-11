<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
giIR ftgen 0, 0, 32, -2, 1
instr 1
  aInput = 0
  aOutput liveconv aInput, giIR, p4, 0, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 -1
i 1 0 .01 3
i 1 0 .01 6
i 1 0 .01 1e20
e
</CsScore>
</CsoundSynthesizer>
