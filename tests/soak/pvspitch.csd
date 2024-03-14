<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pvspitch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Richard Boulanger & Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giwave ftgen 0, 0, 4096, 10, 1, 0.5, 0.333, 0.25, 0.2, 0.1666

instr 1

ifftsize = 1024
iwtype = 1                                              ; hanning window
a1 soundin "flute.aiff"
fsig pvsanal a1, ifftsize, ifftsize/4, ifftsize, iwtype
kfr, kamp pvspitch fsig, p4                             ; estimate pitch, use treshold settings
adm poscil kamp, kfr, giwave                            ; sawtooth gets pitch from the flute
outs adm, adm

endin

</CsInstruments>
<CsScore>
;       treshold
i 1 0 3    .1
i 1 3 3    .01
i 1 6 3    .001
e
</CsScore>
</CsoundSynthesizer>
