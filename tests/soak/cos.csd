<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cos.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
icos1     =          cos(0) ;cosine of 0 is 1
icos2     =          cos($M_PI_2) ;cosine of pi/2 (1.5707...) is 0
icos3     =          cos($M_PI) ;cosine of pi (3.1415...) is -1
icos4     =          cos($M_PI_2 * 3) ;cosine of 3/2pi (4.7123...) is 0
icos5     =          cos($M_PI * 2) ;cosine of 2pi (6.2831...) is 1
icos6     =          cos($M_PI * 4) ;cosine of 4pi is also 1 as it is periodically to 2pi
          print      icos1, icos2, icos3, icos4, icos5, icos6
endin

instr 2 ;cos used in panning, after an example from Hans Mikelson
aout      vco2       0.8, 220 ; sawtooth
kpan      linseg     p4, p3, p5 ;0 = left, 1 = right
kpan      =          kpan*$M_PI_2 ;range 0-1 becomes 0-pi/2
kpanl     =          cos(kpan)
kpanr     =          sin(kpan)
          outs       aout*kpanl, aout*kpanr
endin

</CsInstruments>
<CsScore>
i 1 0 0
i 2 0 5 0 1 ;move left to right
i 2 6 5 1 0 ;move right to left
e
</CsScore>
</CsoundSynthesizer>