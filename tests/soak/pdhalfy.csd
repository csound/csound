<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pdhalfy.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 5

    idur        = p3
    iamp        = p4
    ifreq       = p5
    iamtinit    = p6      ; initial amount of phase distortion
    iatt        = p7      ; attack time
    isuslvl     = p8      ; sustain amplitude
    idistdec    = p9      ; time for distortion amount to reach zero
    itable      = p10
    
    idec        =  idistdec - iatt
    irel        =  .05
    isus        =  idur - (idistdec + irel)
    
    aenv        linseg      0, iatt, 1.0, idec, isuslvl, isus, isuslvl, irel, 0, 0, 0
    kamount     linseg      -iamtinit, idistdec, 0.0, idur-idistdec, 0.0
    aosc        phasor      ifreq
    apd         pdhalfy     aosc, kamount
    aout        tablei      apd, itable, 1
    
                outs        aenv*aout*iamp, aenv*aout*iamp       
endin

</CsInstruments>
<CsScore>
f1 0 16385 10 1                 ; sine
f3 0 16385  9 1 1 270           ; inverted cosine

; descending "just blues" scale

; pdhalfy with cosine table
t 0 100
i5 0 .333 .6    512     1.0   .02  0.5  .12   3
i. + .    .     448     <
i. + .    .     384     <
i. + .    .     358.4   <
i. + .    .     341.33  <
i. + .    .     298.67  <
i. + 2    .     256     0.5
s

; pdhalfy with sine table
t 0 100
i5 0 .333 .6    512     1.0   .001 0.1  .07   1
i. + .    .     448     <
i. + .    .     384     <
i. + .    .     358.4   <
i. + .    .     341.33  <
i. + .    .     298.67  <
i. + 2    .     256     0.5
e
</CsScore>
</CsoundSynthesizer>
