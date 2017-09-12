<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o ftconv.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr      =  48000
ksmps   =  32
nchnls  =  2
0dbfs   =  1

garvb   init 0
gaW     init 0
gaX     init 0
gaY     init 0

itmp    ftgen   1, 0, 64, -2, 2, 40, -1, -1, -1, 123,           \
               1, 13.000, 0.05, 0.85, 20000.0, 0.0, 0.50, 2,   \
               1,  2.000, 0.05, 0.85, 20000.0, 0.0, 0.25, 2,   \
               1, 16.000, 0.05, 0.85, 20000.0, 0.0, 0.35, 2,   \
               1,  9.000, 0.05, 0.85, 20000.0, 0.0, 0.35, 2,   \
               1, 12.000, 0.05, 0.85, 20000.0, 0.0, 0.35, 2,   \
               1,  8.000, 0.05, 0.85, 20000.0, 0.0, 0.35, 2

itmp    ftgen 2, 0, 262144, -2, 0
       spat3dt 2, -0.2, 1, 0, 1, 1, 2, 0.005

itmp    ftgen 3, 0, 262144, -52, 3, 2, 0, 4, 2, 1, 4, 2, 2, 4

       instr 1

a1      vco2 1, 440, 10
kfrq    port 100, 0.008, 20000
a1      butterlp a1, kfrq
a2      linseg 0, 0.003, 1, 0.01, 0.7, 0.005, 0, 1, 0
a1      =  a1 * a2 * 2
       denorm a1
       vincr garvb, a1
aw, ax, ay, az  spat3di a1, p4, p5, p6, 1, 1, 2
       vincr gaW, aw
       vincr gaX, ax
       vincr gaY, ay

       endin

       instr 2

       denorm garvb
; skip as many samples as possible without truncating the IR
arW, arX, arY   ftconv garvb, 3, 2048, 2048, (65536 - 2048)
aW      =  gaW + arW
aX      =  gaX + arX
aY      =  gaY + arY
garvb   =  0
gaW     =  0
gaX     =  0
gaY     =  0

aWre, aWim      hilbert aW
aXre, aXim      hilbert aX
aYre, aYim      hilbert aY
aWXr    =  0.0928*aXre + 0.4699*aWre
aWXiYr  =  0.2550*aXim - 0.1710*aWim + 0.3277*aYre
aL      =  aWXr + aWXiYr
aR      =  aWXr - aWXiYr

       outs aL, aR

       endin

</CsInstruments>
<CsScore>

i 1 0 0.5  0.0  2.0 -0.8
i 1 1 0.5  1.4  1.4 -0.6
i 1 2 0.5  2.0  0.0 -0.4
i 1 3 0.5  1.4 -1.4 -0.2
i 1 4 0.5  0.0 -2.0  0.0
i 1 5 0.5 -1.4 -1.4  0.2
i 1 6 0.5 -2.0  0.0  0.4
i 1 7 0.5 -1.4  1.4  0.6
i 1 8 0.5  0.0  2.0  0.8
i 2 0 10
e

</CsScore>
</CsoundSynthesizer>