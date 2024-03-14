<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xyscale.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
; In the instr 1  using f1 and f2 you reach the 4 corners (values)
k00 = p4
k10 = p5
k01 = p6
k11 = p7

kx oscil 1, p8, p9
ky oscil 1, p10, p11

kout1 xyscale kx, ky, k00, k10, k01, k11 
kout2 xyscale kx, ky, k00*3/2, k10*4/3, k01*5/4, k11*6/5

a1 buzz 0.2, kout1, 8, 3
a2 buzz 0.2, kout2, 4, 3

outs a1, a2

endin

instr 2
; In the instr 2 setting the first value to 0 or 1 (p8 & p9) you can start from the corner  (value) you want

k00 = p4
k10 = p5
k01 = p6
k11 = p7

kx randomh 0, 1, 2, 2, p8 ; p8 is the X starting value
ky randomh 0, 1, 2, 2, p9 ; p9 is the Y starting value

kout1 xyscale kx, ky, k00, k10, k01, k11 
kout2 xyscale kx, ky, k00*3/2, k10*4/3, k01*5/4, k11*6/5

a1 buzz 0.2, kout1, 8, 3
a2 buzz 0.2, kout2, 4, 3

outs a1, a2

endin


</CsInstruments>
<CsScore>


f1 0 1024 -7 0 400 0 100 1 400 1 124 0
f2 0 1024 -7 0 200 0 100 1 400 1 100 0 224 1
f3 0 1024 10 1 

i1 0 10 300 400 500 600 0.3 1 0.2 2
i2 10 10 300 410 520 630 0 0

e


</CsScore>
</CsoundSynthesizer>
