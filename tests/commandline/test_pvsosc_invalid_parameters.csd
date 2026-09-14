<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  fSignal pvsosc .5, 512, 4, p4, p5, p6, p7, p8
endin
instr 2
  fSignal pvsosc .5, p4, 1, 128, 32
endin
</CsInstruments>
<CsScore>
; Nine invalid initial settings and two invalid frequencies.
i 1 0 .05 0 32 128 1 0
i 1 0 .05 3 32 128 1 0
i 1 0 .05 1e30 32 128 1 0
i 1 0 .05 128 -1 128 1 0
i 1 0 .05 128 1e30 128 1 0
i 1 0 .05 128 16 128 1 0
i 1 0 .05 128 32 -1 1 0
i 1 0 .05 128 32 128 1e30 0
i 1 0 .05 128 32 128 1 1
i 2 0 .05 -1
i 2 0 .05 1e-30
e
</CsScore>
</CsoundSynthesizer>
