<CsoundSynthesizer>

<CsOptions>

; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o Ftmorf_Cucchi.wav -W ;;; for file output any platform

; By Stefano Cucchi - 2022

</CsOptions>

<CsInstruments>

sr     =        48000
kr     =        4800
ksmps  =        10
nchnls =        2
0dbfs = 1


garevL init 0
garevR init 0

instr 1 


kgenenvelop linseg 0, 0.3, 0.3, p3-0.6, 0.2, 0.3, 0

kftndx1 linseg 0, p3, 7
iresfn1 = 2
iftfn1 = 1
ftmorf kftndx1, iftfn1, iresfn1
asuono1 oscili 0.5, p4, 2
outch 1, asuono1 * kgenenvelop

kftndx2 linseg 7, p3*0.5, 0, p3*0.5, 7
iresfn2 = 2
iftfn2 = 1
ftmorf kftndx2, iftfn2, iresfn2
asuono2 oscili 0.5, p5, 2
outch 2, asuono2 * kgenenvelop

kftndx3 linseg 0, p3*0.2, 7, p3*0.8, 0
iresfn3 = 2
iftfn3 = 1
ftmorf kftndx3, iftfn3, iresfn3
asuono3 oscili 0.5, p6, 2
outch 1, asuono3 * kgenenvelop


kftndx4 linseg 3, p3*0.2, 7, p3*0.8, 0
iresfn4 = 2
iftfn4 = 1
ftmorf kftndx4, iftfn4, iresfn4
asuono4 oscili 0.5, p7, 2
outch 2, asuono4 * kgenenvelop


garevL = garevL + (asuono1+asuono3)*kgenenvelop
garevR = garevR + (asuono2+asuono4)*kgenenvelop


endin


instr 10

arevL reverb2 garevL, 2.4, 0.4
arevR reverb2 garevR, 2.2, 0.8

outch 1, arevR*0.2
outch 2, arevL*0.2


clear garevL
clear garevR

endin

</CsInstruments>
 
<CsScore> 

f1 0 8 -2 3 4 5 6 7 8 9 10 ; 
f2 0 4097 10 1 /*contents of f4 dont matter */

f3 0 4097 10 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
f4 0 4097 10 1 0 1
f5 0 4097 10 1 0 1 0 1 0 1 0 1 0 0 0 0 0 1  
f6 0 4097 -7  1 2000 1 10 -1 2000 -1 87 1
f7 0 4097 10 1 1 1 1 1 1 0 0 1 0 1 0 1 1 
f8 0 4097 10 1 1 1 0 1 1 0 0 0 1 0 1 1 
f9 0 4097 -7  1 2000 -1 10 1 2000 -1 87 1
f10 0 4097 10 1 0 0 1 0 1 0 1 0 1 0 1 0 1 0 1 1 1 1 1 1


i1 0 20 57 57.2  57.7 58.1
i 10 0 25

e 3

</CsScore>

</CsoundSynthesizer>