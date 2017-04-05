<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pdhalf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 4

    idur        = p3
    iamp        = p4
    ifreq       = p5
    itable      = p6
    
    aenv        linseg      0, .001, 1.0, idur - .051, 1.0, .05, 0
    aosc        phasor      ifreq
    kamount     linseg      0.0, 0.02, -0.99, 0.05, -0.9, idur-0.06, 0.0
    apd         pdhalf      aosc, kamount
    aout        tablei      apd, itable, 1
    
                outs         aenv*aout*iamp, aenv*aout*iamp     
endin

</CsInstruments>
<CsScore>
f1 0 16385 10 1
f2 0 16385 10 1 .5 .3333 .25 .5
f3 0 16385  9 1 1 270           ; inverted cosine

; descending "just blues" scale

; pdhalf with cosine table
; (imitates the CZ-101 "sawtooth waveform")
t 0 100
i4 0 3   .6     512     3
i. + .    .     448
i. + .    .     384
i. + .    .     358.4
i. + .    .     341.33
i. + .    .     298.67
i. + 5    .     256
s
; pdhalf with a sine table
t 0 120
i4 0 3   .6     512     1
i. + .    .     448
i. + .    .     384
i. + .    .     358.4
i. + .    .     341.33
i. + .    .     298.67
i. + 5    .     256
s
; pdhalf with a sawtooth-like table
t 0 150
i4 0 3   .6     512     2
i. + .    .     448
i. + .    .     384
i. + .    .     358.4
i. + .    .     341.33
i. + .    .     298.67
i. + 5    .     256
e
</CsScore>
</CsoundSynthesizer>
