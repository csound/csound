<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kBlock init 0
  kX = p4
  kY = p5
  ; A weighted sum gives an independent bilinear reference.
  kExpected = (1-kX)*(1-kY)*p6 + kX*(1-kY)*p7 + (1-kX)*kY*p8 + kX*kY*p9
  kFixed xyscale kX, kY, p6, p7, p8, p9
  ; Change the corner values on each control cycle.
  k00 = p6 + kBlock
  k10 = p7 - kBlock
  k01 = p8 + 2*kBlock
  k11 = p9 - 3*kBlock
  kDynamicExpected = (1-kX)*(1-kY)*k00 + kX*(1-kY)*k10 + (1-kX)*kY*k01 + kX*kY*k11
  kDynamic xyscale kX, kY, k00, k10, k01, k11
  kAlias = kX
  kAlias xyscale kAlias, kY, k00, k10, k01, k11
  if !(abs(kFixed-kExpected) < .00001) || \
      !(abs(kDynamic-kDynamicExpected) < .00001) || \
      !(abs(kAlias-kDynamicExpected) < .00001) then
    printks "FAIL xyscale at (%g, %g): fixed %g expected %g, dynamic %g alias %g expected %g\n", \
        0, kX, kY, kFixed, kExpected, kDynamic, kAlias, kDynamicExpected
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "FAIL xyscale checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; All corners, then asymmetric interior points and different surfaces.
i 1 0    .0625 0    0    0 10 20 30
i 1 .125 .0625 1    0    0 10 20 30
i 1 .25  .0625 0    1    0 10 20 30
i 1 .375 .0625 1    1    0 10 20 30
i 1 .5   .0625 .25  .75  0 10 20 30
i 1 .625 .0625 .75  .25 -4  3  9 -2
i 1 .75  .0625 .5   .5   1  2  4  8
i 1 .875 .0625 .125 .625 1  2  4  8
i 99 1 .015625
e
</CsScore>
</CsoundSynthesizer>
