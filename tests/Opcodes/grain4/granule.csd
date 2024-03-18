<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac    ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o granule.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kenv linseg 0,0.5,1,(p3-0.5),1      ; envelope with attack time of .5 seconds

iamp = p4
ivoice = p5
iratio = p6
imode = p7
ithd = p8
ifn = p9
ipshift = p10
igskip = p11
igskipos = p12
ilenght = p13
kgap = p14
igapos = p15
kgsize = p16
igsize_os = p17
iatt = p18
idec = p19
iseed = p20
ipitch1 = p21
ipitch2 = p22
ipitch3 = p23
ipitch4 = p24

a1  granule iamp*kenv, ivoice, iratio, imode, ithd, ifn, ipshift ,igskip ,igskipos , ilenght, kgap ,igapos, kgsize, igsize_os, iatt, idec, iseed,      ipitch1, ipitch2, ipitch3, ipitch4
a2  granule iamp*kenv, ivoice, iratio, imode, ithd, ifn, ipshift ,igskip ,igskipos , ilenght, kgap ,igapos, kgsize, igsize_os, iatt, idec, iseed+0.17, ipitch1, ipitch2, ipitch3, ipitch4
outs a1,a2

endin

</CsInstruments>
<CsScore>
f1      0 0 1 "marimba.aif" 0 0 0
f2      0 0 1 "fox.wav" 0 0 0

; both samples are played back 2 x slower

;         p4 p5 p6 p7 p8 p9 p10 p11 p12  p13 p14  p15 p16   p17 p18 p19  [p20   p21 p22  p23  p24]
i1 0  10 .25 64 1 0   0  1   4  0  0.005  1  0.01  0  0.02   50 25  25    0.39  1  1.42 0.29  2 ;part of marimba
i1 11 20 .37 9  1 0   0  2   0  0  0.005  1  0.01  0  0.02   50 25  25    0.39  1  1.42 0.29  2 ;part of fox
      
e
</CsScore>
</CsoundSynthesizer>
