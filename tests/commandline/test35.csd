<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

          instr 8

idur,iamp,iskiptime,iattack,irelease,irvbtime,irvbgain  passign   3

kamp      linen     iamp, iattack, idur, irelease
;asig      soundin   "fox.wav", iskiptime
asig = 1
arampsig  =         kamp * asig
aeffect   reverb    asig, irvbtime
arvbretrn =         aeffect * irvbgain

          out       arampsig + arvbretrn

          endin

</CsInstruments>
<CsScore>

;ins strt dur  amp  skip atk  rel       rvbt rvbgain
i8   0    .2 .3   0    .03  .1        1.5  .3


e
</CsScore>
</CsoundSynthesizer>
