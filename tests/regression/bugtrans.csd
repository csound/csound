<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs  = 1

instr 1
turnoff2 p4, 0, 1
turnoff
endin

instr 2; linsegr: works
asin oscils .2, 500, 0
kenv linsegr 1, p3, 1, 1, 0
      out asin * kenv
endin

instr 3; transegr: does not work
asin oscils .2, 500, 0
kenv transegr 1, p3/2, 0, 1, p3/2, 0, 0.5, 0.5, 0, 0
      out asin * kenv
endin
</CsInstruments>
<CsScore>
  ;turn off instr 2 after two sec with fade out
i 2 0 5
i 1 2 1 2
  ;should turn off instr 3 after two sec with fade out
  ;but instead produces samples out of range
i 3 4 5
i 1 6 1 3
</CsScore>
</CsoundSynthesizer>
