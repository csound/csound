<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
  km = metro(1)
  kt timeinsts
  ktrig = sc_trig(km, 0.5)
  printks "t=%f  km=%f    ktrig=%f\n", 0.01, kt, km, ktrig
endin

instr 2
  am = upsamp(metro(1))
  aenv = sc_trig(am, 0.5)
  asig pinker
  outch 1, asig*aenv
  outch 2, asig
endin

</CsInstruments>
<CsScore>
i 1 0 10
i 2 0 10

</CsScore>
</CsoundSynthesizer>

