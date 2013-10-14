<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

nchnls    =         2                        ; stereo output

garvbsig  init      0                        ; global "a" variable initialized to 0

          instr 9

idur      =         p3
iamp      =         p4
iskiptime =         p5
iattack   =         p6
irelease  =         p7
ibalance  =         p8                       ; 1 = left, .5 = center, 0 = right
irvbgain  =         p9

kamp      linen     iamp, iattack, idur, irelease
asig      soundin   "fox.wav", iskiptime
arampsig  =         kamp * asig

          outs      arampsig * ibalance, arampsig * (1 - ibalance)

garvbsig  =         garvbsig + arampsig * irvbgain

          endin

          instr 99

irvbtime  =         p4

asig      reverb    garvbsig,  irvbtime      ; put global sig into reverb

          outs      asig, asig

garvbsig  =         0                        ; then clear it

          endin

</CsInstruments>
<CsScore>

;ins strt dur  rvbtime
i99  0    15   2.2

;ins strt dur  amp  skip atk  rel  blnce(0-1)     rvbsend
i9   0    1.2  .5   0    .02  .1   1              .2
i9   2    1.4  .5   0    .03  .1   0              .3
i9   3.5  2.28 .5   0    .9   .1   .5             .1
i9   4.5  2.28 .5   0    1.2  .1   0              .2
i9   5    2.28 .5   0    .2   .1   1              .3
i9   9    2.28 .7   0    .1   .1   .5             .03

e
</CsScore>
</CsoundSynthesizer>
