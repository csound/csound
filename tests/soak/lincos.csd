<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Example file for lincos.csd

/*
  lincos

  similar to cosseg, but with an explicit input for time
  lincos can be used ease-in / out any linear ramp

  ky  lincos kx, ky0, ky1, kx0=0, kx1=1
  iy  lincos ix, iy0, iy1, ix0=0, ix1=1

*/


seed 0

instr 1
  ; Map a value within the range 1-3 to the range 0-10.
  iy lincos 1.5, 0, 10, 1, 3
  print iy
  kx line 1, p3, 3
  ky lincos kx, 0, 10, 1, 3
  printks "kx: %f   ky: %f \n", 1/kr, kx, ky
endin

instr 2
  ; lincos can be used to create amplitude or pitch envelopes
  ktrig init 0
  krnd dust 1, 1
  ktrig = lineto(tirghold(krnd & ~ktrig, 0.5), 1)
  kpitch = lincos:k(ktrig, 60, 61)
  a0 oscili 0.7, mtof(kpitch)

  kfade  lincos linsegr(0, 1.5, 1, 1.5, 0), 0, 1
  kcresc lincos ktrig, 0.25, 1
  outch 1, a0 * interp(kfade * kcresc)
endin

</CsInstruments>
<CsScore>

i 1 0 0.2
i 2 0 20

</CsScore>
</CsoundSynthesizer>
