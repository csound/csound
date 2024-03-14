<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out
;-iadc ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o turnon.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Stefano Cucchi 2020 and Richard Boulanger

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gasend1 init 0
gasend2 init 0

turnon 100
turnon 101

instr 1

abar barmodel 2, 2, p4, 0.000001, 0.0103, 20, p5, p6, p7
kenvelope expseg 1, p3, .01
abar = abar * kenvelope * p8
; NO local output

gasend1 = gasend1 + abar*p8
gasend2 = gasend2 + abar*p8

endin


instr 2

idenStart = p4
kdensity expseg p4, p3, 0.001
apulse mpulse 1, kdensity
kenvelope expseg 1, p3, .001
apulse = apulse * kenvelope * p5
; NO local output

gasend1 = gasend1 + apulse
gasend2 = gasend2 + apulse

endin


instr dry

idryLeft = p4
idryRight = p5
kgate adsr .01,.01,1,0
outs (gasend1*kgate) * p4, (gasend2*kgate) * p5

endin


instr 100

aphaser phaser1 gasend1, 4700, 1000, -0.8
arevphaser reverb aphaser, 1.618
outs arevphaser * 0.2, arevphaser * 0.2
clear gasend1

endin


instr 101

afold fold gasend2, 26
adelfoldL multitap afold, 0.62, 0.52, 1.19, 0.44
adelfoldR multitap afold, 0.94, 0.64, 1.42, 0.54
outs adelfoldL * 0.5, adelfoldR * 0.5
clear gasend2

endin

</CsInstruments>
<CsScore>



i2 0 6 .07  1

i "dry" 0 2 .87 .2

i1 3 4 10 0.13 300 0.3 0.8

i2 7 5 .007  1

i "dry" 7 3 .12 .86

i1 11 9 09 0.09 400 0.23 0.78

i2 15 9 .01 1


i "dry" 15 12 .94 .09


s


i "dry" 0 26.9 .68 .48 ; lowered

i 2 0.0 4 .07  1
i 2 0.1 4 .02  1

i 1 0.21 1 08 0.13 300 0.3 0.8 
i 1 2 2 14 0.02 500 0.2 0.7 

i 2 2 5 .06  0.8 ; lowered
i 2 4 5 .05  0.7 ; lowered
i 2 4 6 .01  0.6 ; lowered


i 1 4.3 3 9 0.08 1145 0.1 0.5 ; lowered
i 1 4.5 3 11 0.10 245 0.2 0.4 ; lowered

i 1 8.00 7 08 0.11 125 0.1 0.6
i 1 8.01 6 28 0.13 280 0.3 0.8
i 1 8.02 5 39 0.02 600 0.2 0.7
i 1 8.03 4 39 0.02 600 0.2 0.7

i 2 8.24 18 .09 1
i 2 8.75 11 .05 1
i 2 8.99 4 .01 1



i 1 18 9 8.1 0.12 105 0.12 0.56
i 1 18.01 8 8.8 0.13 205 0.52 0.46

i 1 18.02 7 8.2 0.15 745 0.42 0.26
i 1 18.03 11 8.3 0.14 905 0.62 0.36

i "dry" 25 .85 .32 .38

i 2 25 5 .71 1

i "dry" 27 .74 .62 .68

i 2 27 6 .11 1

i "dry" 32.3 .42 .32 .38

i 2 32.3 14 .005  1


i 1 36 13 9.9 0.233 333 0.23 0.63
i 1 39 12 8.9 0.323 403 0.34 0.53
i 1 42 10 7.8 0.123 283 0.42 0.73

i 1 48.01 9 8.11 0.122 108 0.13 0.54
i 1 48.03 8 8.82 0.135 208 0.50 0.44
i 1 48.05 7 8.23 0.158 748 0.40 0.24
i 1 48.08 11 8.35 0.147 908 0.60 0.34

e 60 ; score extended to allow for reverb and multitap tails to ring out

; keeping instr 100 and instr 101 "turned on" for 60 seconds

</CsScore>
</CsoundSynthesizer>
