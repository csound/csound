<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

giSine ftgen 0, 0, 4096, 10, 1

opcode TestXout, aa, ikai
  ifreq, koff, aamp, itbl xin
  koff1 = koff
  koff2 = 2 * koff

  a1 poscil aamp, ifreq+koff1, itbl
  a2 poscil aamp, ifreq+koff2, itbl
  a3 poscil aamp, ifreq-koff1, itbl
  a4 poscil aamp, ifreq-koff2, itbl

  aL = a1 + a3
  aR = a2 + a4

  xout aL, aR
endop

instr 1
  ifreq = 440
  koff = 0.004
  aenv linen 0.5, 0.01, p3, 0.01

  aL, aR TestXout ifreq, koff, aenv, giSine
  out aL, aR
endin

instr 2
  ifreq = 440
  koff = 0.004
  aenv linen 0.5, 0.01, p3, 0.01

  out TestXout(ifreq, koff, aenv, giSine)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
</CsScore>
</CsoundSynthesizer>
