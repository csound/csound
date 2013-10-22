<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o babo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Nicola Bernardini */

sr = 44100
ksmps = 32
nchnls = 2

; minimal babo instrument
;
instr 1
       ix     = p4  ; x position of source
       iy     = p5  ; y position of source
       iz     = p6  ; z position of source
       ixsize = p7  ; width  of the resonator
       iysize = p8  ; depth  of the resonator
       izsize = p9  ; height of the resonator

ainput soundin "beats.wav"

al,ar  babo    ainput*0.7, ix, iy, iz, ixsize, iysize, izsize

       outs    al,ar
endin


</CsInstruments>
<CsScore>

/* Written by Nicola Bernardini */
; simple babo usage:
;
;p4     : x position of source
;p5     : y position of source
;p6     : z position of source
;p7     : width  of the resonator
;p8     : depth  of the resonator
;p9     : height of the resonator
;
i  1  0  20 6  4  3    14.39 11.86 10
;           ^^^^^^^    ^^^^^^^^^^^^^^
;           |||||||    ++++++++++++++: optimal room dims according to
;           |||||||                    Milner and Bernard JASA 85(2), 1989
;           +++++++++: source position
e


</CsScore>
</CsoundSynthesizer>
