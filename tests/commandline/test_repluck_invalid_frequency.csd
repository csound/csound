<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aout wgpluck2 .5, .2, p4, .5, .5
endin

instr 2
  aexcite = 0
  aout repluck .5, .2, p4, .5, .5, aexcite
endin
</CsInstruments>
<CsScore>
i 1 0 .01 96000
i 2 0 .01 96000
i 1 0 .01 0
i 2 0 .01 0
i 1 0 .01 -440
i 2 0 .01 -440
i 1 0 .01 1e-30
i 2 0 .01 1e-30
</CsScore>
</CsoundSynthesizer>
