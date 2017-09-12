<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o babo_expert.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Nicola Bernardini */

sr = 44100
ksmps = 32
nchnls = 2

; full blown babo instrument with movement
;
instr 2
  ixstart = p4   ; start x position of source (left-right)
  ixend   = p7   ; end   x position of source
  iystart = p5   ; start y position of source (front-back)
  iyend   = p8   ; end   y position of source
  izstart = p6   ; start z position of source (up-down)
  izend   = p9  ; end   z position of source
  ixsize  = p10  ; width  of the resonator
  iysize  = p11  ; depth  of the resonator
  izsize  = p12  ; height of the resonator
  idiff   = p13  ; diffusion coefficient
  iexpert = p14  ; power user values stored in this function

ainput    soundin "beats.wav"
ksource_x line    ixstart, p3, ixend
ksource_y line    iystart, p3, iyend
ksource_z line    izstart, p3, izend

al,ar     babo    ainput*0.7, ksource_x, ksource_y, ksource_z, ixsize, iysize, izsize, idiff, iexpert

          outs    al,ar
endin


</CsInstruments>
<CsScore>

/* Written by Nicola Bernardini */
; full blown instrument
;p4         : start x position of source (left-right)
;p5         : end   x position of source
;p6         : start y position of source (front-back)
;p7         : end   y position of source
;p8         : start z position of source (up-down)
;p9         : end   z position of source
;p10        : width  of the resonator
;p11        : depth  of the resonator
;p12        : height of the resonator
;p13        : diffusion coefficient
;p14        : power user values stored in this function

;         decay  hidecay rx ry rz rdistance direct early_diff
f1  0 8 -2  0.95   0.95   0  0  0    0.3     0.5      0.8  ; brighter
f2  0 8 -2  0.95   0.5    0  0  0    0.3     0.5      0.8  ; default (to be set as)
f3  0 8 -2  0.95   0.01   0  0  0    0.3     0.5      0.8  ; darker
f4  0 8 -2  0.95   0.7    0  0  0    0.3     0.1      0.4  ; to hear the effect of diffusion
f5  0 8 -2  0.9    0.5    0  0  0    0.3     2.0      0.98 ; to hear the movement
f6  0 8 -2  0.99   0.1    0  0  0    0.3     0.5      0.8  ; default vals
;        ^
;         ----- gen. number: negative to avoid rescaling


i2 0 10  6  4  3   6   4  3  14.39  11.86  10    1  6 ; defaults
i2 +  4  6  4  3   6   4  3  14.39  11.86  10    1  1 ; hear brightness 1
i2 +  4  6  4  3  -6  -4  3  14.39  11.86  10    1  2 ; hear brightness 2
i2 +  4  6  4  3  -6  -4  3  14.39  11.86  10    1  3 ; hear brightness 3
i2 +  3 .6 .4 .3 -.6 -.4 .3  1.439  1.186  1.0 0.0  4 ; hear diffusion 1
i2 +  3 .6 .4 .3 -.6 -.4 .3  1.439  1.186  1.0 1.0  4 ; hear diffusion 2
i2 +  4 12  4  3 -12  -4 -3  24.39  21.86  20    1  5 ; hear movement
;
i2 +  4  6  4  3   6   4  3  14.39  11.86   10   1  1 ; hear brightness 1
i2 +  4  6  4  3  -6  -4  3  14.39  11.86   10   1  2 ; hear brightness 2
i2 +  4  6  4  3  -6  -4  3  14.39  11.86   10   1  3 ; hear brightness 3
i2 +  3 .6 .4 .3 -.6 -.4 .3  1.439  1.186  1.0 0.0  4 ; hear diffusion 1
i2 +  3 .6 .4 .3 -.6 -.4 .3  1.439  1.186  1.0 1.0  4 ; hear diffusion 2
i2 +  4 12  4  3 -12  -4 -3  24.39  21.86   20   1  5 ; hear movement
;       ^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^   ^  ^
;       |||||||||||||||||||  |||||||||||||||||   |   --: expert values function
;       |||||||||||||||||||  |||||||||||||||||   +--: diffusion
;       |||||||||||||||||||  ----------------: optimal room dims according to Milner and Bernard JASA 85(2), 1989
;       |||||||||||||||||||
;       --------------------: source position start and end
e


</CsScore>
</CsoundSynthesizer>
