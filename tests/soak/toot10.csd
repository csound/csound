<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

nchnls    =         2

garvbsig init       0


          instr 10

iattack   =         .01
irelease  =         .2
iwhite    =         22050
idur      =         p3
iamp      =         p4
iswpstart =         p5
isweepend =         p6
ibndwidth =         p7
ibalance  =         p8                  ; 1 = left, .5 = center, 0 = right
irvbgain  =         p9

kamp      linen     iamp, iattack, idur, irelease
ksweep    line      iswpstart, idur, isweepend
asig      rand      iwhite
afilt     reson     asig, ksweep, ibndwidth
arampsig  =         kamp * afilt

          outs      arampsig * ibalance, arampsig * (1 - ibalance)

garvbsig  =         garvbsig + arampsig * p9

          endin

          instr     100

irvbtime  =         p4
asig      reverb    garvbsig,  irvbtime
          outs      asig, asig
garvbsig  =         0

          endin


</CsInstruments>
<CsScore>

;ins strt dur  rvbtime
i100 0    15   1.1
i100 15   10   5

;ins strt dur  amp   stsw   ndsw   bdw  bal(0-1)  rvsnd
i10  0    2    .01   5000   500    20   .15        .1
i10  3    1    .01   1500   5000   30   .95        .1
i10  5    2    .01   850    1100   40   .45        .1
i10  8    2    .01   1100   8000   50   .05        .1
i10  8    .5   .01   5000   1000   30   .35        .2
i10  9    .5   .01   1000   8000   40   .75        .1
i10  11   .5   .01   500    2100   50   .14        .2
i10  12   .5   .01   2100   1220   75   .96        .1
i10  13   .5   .01   1700   3500   100  .45        .2
i10  15   5    .005  8000   800    60   .85        .1

e
</CsScore>
</CsoundSynthesizer>
