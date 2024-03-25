<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xscanmap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
; example by Richard Boulanger
seed 0

instr 1

a0    = 0
iamp  = p4
ifrq  = p5	
iscanrate   = p6
iposgain    = p7
ivelgain    = p8	
inode       = p9

xscanu 1, iscanrate, 6, 2, "128-stringcircular.XmatrxT", 4, 5, 2, .1, .1, -.01, .1, .5, 0, 0, a0, 0, 0  		
k1,k2 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k3,k4 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k5,k6 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k7,k8 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k9,k10  xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k11,k12 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k13,k14 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k15,k16 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))

a1 poscil3 iamp,rnd(ifrq)+k1
a2 poscil3 iamp,rnd(ifrq)+k2
a3 poscil3 iamp,rnd(ifrq)+k3
a4 poscil3 iamp,rnd(ifrq)+k4
a5 poscil3 iamp,rnd(ifrq)+k5
a6 poscil3 iamp,rnd(ifrq)+k6
a7 poscil3 iamp,rnd(ifrq)+k7
a8 poscil3 iamp,rnd(ifrq)+k8
a9 poscil3 iamp,rnd(ifrq)+k9
a10 poscil3 iamp,rnd(ifrq)+k10
a11 poscil3 iamp,rnd(ifrq)+k11
a12 poscil3 iamp,rnd(ifrq)+k12
a13 poscil3 iamp,rnd(ifrq)+k13
a14 poscil3 iamp,rnd(ifrq)+k14
a15 poscil3 iamp,rnd(ifrq)+k15
a16 poscil3 iamp,rnd(ifrq)+k16

aL sum a1,a3,a5,a7,a9,a11,a13,a15
aR sum a2,a4,a6,a8,a10,a12,a14,a16

kenv  adsr .3,.1,.8,.3
      outs  (aL*kenv)*.6, (aR*kenv)*.6

endin

instr 2 ; with audio injection	

ain   diskin2 "fox.wav", 1, 0, 1	
a0    = ain/32000 						
iamp  = p4
ifrq  = p5	
iscanrate   = p6
iposgain    = p7
ivelgain    = p8	
inode       = p9					

xscanu 1, iscanrate, 6, 2, "128-stringcircular.XmatrxT", 4, 5, 2, .1, .1, -.01, .1, .5, 0, 0, a0, 0, 0  		
k1,k2 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k3,k4 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k5,k6 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k7,k8 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k9,k10  xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k11,k12 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k13,k14 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))
k15,k16 xscanmap 0, iposgain, ivelgain, int(abs((rnd(inode))))

a1 poscil3 iamp,rnd(ifrq)+k1
a2 poscil3 iamp,rnd(ifrq)+k2
a3 poscil3 iamp,rnd(ifrq)+k3
a4 poscil3 iamp,rnd(ifrq)+k4
a5 poscil3 iamp,rnd(ifrq)+k5
a6 poscil3 iamp,rnd(ifrq)+k6
a7 poscil3 iamp,rnd(ifrq)+k7
a8 poscil3 iamp,rnd(ifrq)+k8
a9 poscil3 iamp,rnd(ifrq)+k9
a10 poscil3 iamp,rnd(ifrq)+k10
a11 poscil3 iamp,rnd(ifrq)+k11
a12 poscil3 iamp,rnd(ifrq)+k12
a13 poscil3 iamp,rnd(ifrq)+k13
a14 poscil3 iamp,rnd(ifrq)+k14
a15 poscil3 iamp,rnd(ifrq)+k15
a16 poscil3 iamp,rnd(ifrq)+k16

aL sum a1,a3,a5,a7,a9,a11,a13,a15
aR sum a2,a4,a6,a8,a10,a12,a14,a16

kenv  adsr .3,.1,.8,.3
      outs  (aL*kenv)*.6, (aR*kenv)*.6

endin 
</CsInstruments>
<CsScore>
f1 0 128 10 1                 ; Initial condition
f2 0 128 -7 1 128 .5          ; Masses
f4 0 128 -7 .2 64 1 64 .4     ; Centering force 
f5 0 128 -7 1 64 .1 64 .6     ; Damping
f6 0 128 -7 2 64 .1 64 4      ; Initial velocity

; ins strt dur amp frq  scanrate   pgain  vgain  inode
i 1   0    3   .1  500  .1         200    800     27                                  
i 1   +    .   .1  500  .1         300    900     27                                  
i 1   +    .   .1  500  .1         400    1000    27                                  
i 1   +    .   .1  500  .1         500    2100    27                                  
s 
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 1   0    3   .1  1500  .21         500    1100    47                                  
i 1   +    .   .1  500  .31         1500    1800    47                                  
i 1   +    .   .1  1000  .41         2500    2800    47                                  
i 1   +    .   .1  200  .51         4500    4800    47                                  
s
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 1   0    2   .1  4500  .1         1800    1800  97                                  
i 1   +    .   .1  4500  .1         1800    1800  97                                  
i 1   +    .   .1  4500  .1         1800    1800  97                                  
i 1   +    .   .1  4500  .1         1800    1800  97                                  
s
; ins strt dur amp frq  scanrate   pgain  vgain  inode
i 1   0    3   .1  500  .01         200    800    27                                  
i 1   +    .   .1  500  .01         200    800    27                                  
i 1   +    .   .1  500  .01         200    800    27                                  
i 1   +    .   .1  500  .01         200    800    27                                  
s 
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 1   0    3   .1  1500  .001         500    800    47                                  
i 1   +    .   .1  1500  .001         500    800    47                                  
i 1   +    .   .1  1500  .001         500    800    47                                  
i 1   +    .   .1  1500  .001         500    800    47                                  
s
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 1   0    4   .1   500  .001       2800    4800  97                                  
i 1   +    .   .1  1500  .001       3800    5800  97                                  
i 1   +    .   .1  2500  .001       4800    8800  97                                  
i 1   +    .   .1  3500  .001       5800    9980  97                                  
s
; ins strt dur  amp    frq   scanrate   pgain  vgain  inode
i 1   0    23   .13     400   .001       2800    4800  97                                  
i 1   1    18   .05    1000   .01        800     1800  87                                  
i 1   3    18   .09     200   .001       4800    8800  77                                  
i 1   5    16   .05    2000   .1         5800    9980  47                                  
i 1   8    12   .04    7500   .001       2800    4800  7                                  
i 1   11    8   .04    4000   .01        3800    5800  17                                  
i 1   13    8   .08    1600   .001       4800    8800  27                                  
i 1   15    8   .03    8000   .1        5800    9980  127                                  
s
; ins strt dur amp frq  scanrate   pgain  vgain  inode
i 2   0    3   .1  500  .1          200    800    27                                  
i 2   +    .   .1  500  .21         1200    800    37                                  
i 2   +    .   .1  500  .31         2200    800    77                                  
i 2   +    .   .1  500  .41         3200    800    107                                  
s 
; ins strt dur amp frq  scanrate   pgain  vgain  inode
i 2   0    2   .1  500  .01         200    800    27                                  
i 2   +    .   .1  500  .009         200    800    27                                  
i 2   +    .   .1  500  .005         200    800    27                                  
i 2   +    .   .1  500  .002         200    800    27                                  
s 
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 2   0    1   .1  1500  .001         500    800    47                                  
i 2   +    .   .1  1500  .001         500    800    47                                  
i 2   +    .   .1  1500  .001         500    800    47                                  
i 2   +    .   .1  1500  .001         500    800    47                                  
s
; ins strt dur amp frq   scanrate   pgain  vgain  inode
i 2   0    1   .1   500  .001       2800    4800  97                                  
i 2   +    .   .1  1500  .001       3800    5800  97                                  
i 2   +    .   .1  2500  .001       4800    8800  97                                  
i 2   +    .   .1  3500  .001       5800    9980  97                                  
s
; ins strt dur  amp    frq   scanrate   pgain  vgain  inode
i 2   0    23   .13     400   .001       2800    4800  97                                  
i 2   1    18   .05    1000   .01        800     1800  87                                  
i 2   3    18   .09     200   .001       4800    8800  77                                  
i 2   5    16   .05    2000   .1         5800    9980  47                                  
i 2   8    12   .04    7500   .001       2800    4800  7                                  
i 2   11    8   .04    4000   .01        3800    5800  17                                  
i 2   13    8   .08    1600   .001       4800    8800  27                                  
i 2   15    8   .03    8000   .1         5800    9980  127                                  
e
</CsScore>
</CsoundSynthesizer>
