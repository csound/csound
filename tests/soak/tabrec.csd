<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifn ftgen 0,0,1049576,2,0

; record control signals
instr 1
ktrig_start init    1
koct        rspline 7,10,1,2
kpan        rspline 7,10,0.1,0.9
ktrig_stop  =       1
knumtics    =       kr*p3
            tabrec  ktrig_start,ktrig_stop,knumtics,gifn,koct,kpan
ktrig_start =   0
endin

; play control signals
instr 2
koct,kpan   init    0
ktrig       init    1
knumtics    =       kr*p3
            tabplay ktrig,knumtics,gifn,koct,kpan

ktrig       =       0

asig        poscil 0.1, cpsoct(koct)
aL,aR       pan2    asig,kpan
            outs    aL,aR
endin


</CsInstruments>

<CsScore>
i1 0 10
i2 2 10
i2 4 10
e

</CsScore>

</CsoundSynthesizer>
