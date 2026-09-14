<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
instr 1
  fSignal pvsinit p4, p5, p6, p7, p8
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 16 64 1 0
i 1 0 .01 63 16 64 1 0
i 1 0 .01 2147483648 16 64 1 0
i 1 0 .01 64 -1 64 1 0
i 1 0 .01 64 2147483648 64 1 0
i 1 0 .01 64 16 -1 1 0
i 1 0 .01 64 16 64 2147483648 0
i 1 0 .01 64 16 64 1 3
e
</CsScore>
</CsoundSynthesizer>
