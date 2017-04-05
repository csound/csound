<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o passign.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 8

idur,iamp,iskiptime,iattack,irelease,irvbtime,irvbgain  passign   3

kamp      linen     iamp, iattack, idur, irelease
asig      soundin   "fox.wav", iskiptime
arampsig  =         kamp * asig
aeffect   reverb    asig, irvbtime
arvbretrn =         aeffect * irvbgain
;mix dry & wet signals
          outs      arampsig + arvbretrn, arampsig + arvbretrn

          endin

</CsInstruments>
<CsScore>

;ins strt dur  amp  skip atk  rel       rvbt rvbgain
i8   0    4    .3   0    .03  .1        1.5  .3
i8   4    4    .3   1.6  .1   .1        3.1  .7
i8   8    4    .3   0    .5   .1        2.1  .2
i8   12   4    .4   0    .01  .1        1.1  .1
i8   16   4    .5   0.1  .01  .1        0.1  .1

e
</CsScore>
</CsoundSynthesizer>
