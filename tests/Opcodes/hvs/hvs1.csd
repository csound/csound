<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac     ;      -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o hvs1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr=44100
ksmps=32
nchnls=2
0dbfs = 1

; after the original FLTK example by Gabriel Maldonado and Andres Cabrera
; by Stefano Cucchi and Menno Knevel

instr 1
inumLinesX init 16
inumParms  init 3
iOutTab ftgen   5,0,8, -2,      0
iPosTab ftgen   6,0,32, -2,     3,2,1,0,4,5,6,7,8,9,10, 11, 15, 14, 13, 12
iSnapTab    ftgen   8,0,64, -2,     1,1,1,   2,0,0,  3,2,0,  2,2,2,  5,2,1,  2,3,4,  6,1,7,    0,0,0, \
                              1,3,5,   3,4,4,  1,5,8,  1,1,5,  4,3,2,  3,4,5,  7,6,5,    7,8,9
k1 linseg 0, p3, 1
printk 0.2, k1
;               kx,   inumParms,  inumPointsX,  iOutTab,  iPosTab,  iSnapTab  [, iConfigTab]
        hvs1    k1,  inumParms, inumLinesX, iOutTab, iPosTab, iSnapTab  ;, iConfigTab

k0  tab     0, 5
k1  tab     1, 5
k2  tab     2, 5

printk2 k0
printk2 k1, 10
printk2 k2, 20

aosc1 oscil k0/20, k1*100 + 200, 1
aosc2 oscil k1/20, k2*100 + 200, 1
aosc3 oscil k2/20, k0*100 + 200, 1
aosc4 oscil k1/20, k0*100 + 200, 1
aosc5 oscil k2/20, k1*100 + 200, 1
aosc6 oscil k0/20, k2*100 + 200, 1
outs (aosc1 + aosc2 + aosc3)*0.7, (aosc4 + aosc5 + aosc6)*0.7

    endin
</CsInstruments>
<CsScore>
f1 0 1024 10 1
i1 0 10
</CsScore>
</CsoundSynthesizer>
