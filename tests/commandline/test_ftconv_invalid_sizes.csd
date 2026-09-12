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
  aOutput ftconv aInput, giIR, p4, p5, p6
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 0 0
i 1 0 .01 3 0 0
i 1 0 .01 6 0 0
i 1 0 .01 1e20 0 0
i 1 0 .01 8 1e20 0
i 1 0 .01 8 0 1e20
i 1 0 .01 8 32 0
i 1 0 .01 8 -2147483648 0
e
</CsScore>
</CsoundSynthesizer>
