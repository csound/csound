<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o BuzzFof_Cucchi.wav -W ;;; for file output any platform


; By Stefano Cucchi - 2022

</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 10 
nchnls = 2
0dbfs  = 1


garev1L init 0
garev1R init 0


instr buzz
 
 iDensInit = p4  ; Initial bouncing density
 iDensFinal = p5 ; Final bouncing density
 
 kfe1  expseg iDensInit, p3, iDensFinal
 
 kftndx linseg 0, p3*0.2, 1, p3*0.2, 0, p3*0.6, 1
 iresfn = 2
 iftfn = 1
 ftmorf kftndx, iftfn, iresfn
 kenv expseg 0.0001, p11, p8, p3-p11-p12, p8, p12, 0.0001
 
 asig buzz kenv, kfe1, sr/(2*kfe1), 2
 kfreqformant expseg p6, p3, p7
 aform fofilter asig, kfreqformant, 0.007, 0.04
 

 ifiltergain = 0.015
 aeq pareq aform, kfreqformant, ifiltergain, 0.1, 0
 
 krandompan randi 0.5, 4, p9
 krandompan = krandompan+0.5
 
 aL, aR pan2 aeq*16, krandompan
 
 outch 1, aL 
 outch 2, aR 
 
 adelay1 delay aL, 0.2
 adelay2 delay aR, 0.36
 
 outch 1, adelay2*0.01
 outch 2, adelay1*0.01
 

 garev1L = garev1L + aR * p10
 garev1R = garev1R + aL * p10
 
 endin


instr fof
 
 ifreq1 = cpspch (p4)
 ifreq2 = cpspch (p5)
 ifreq3 = cpspch (p6)
 ifreq4 = cpspch (p7)
 kfreq expseg ifreq1, p3*p8, ifreq2, p3*p9, ifreq3, p3*p10, ifreq4
 
 iAmp = 0.99
 
 krandom1 randomi -3, 3, 4
 krandom2 randomi -30, 30, 8
 kjitter1 jitter 2, 0.2, 5
 kform1 = p11 + krandom1
 koct1 = 2
 kband1 = p12 + krandom2
 kris1 = 0.003
 kdur1 = 0.12
 kdec1 = 0.007
 iolaps1 = 1900
 ifna1 = 15
 
 krandom3 randomi -2, 2.2, 3.8
 krandom4 randomi -25, 26, 2.6
 kjitter2 jitter 3, 0.2, 3
 
 kform2 = p13 + krandom3
 koct2 = 2
 kband2 = p14 + krandom4
 kris2 = 0.0003
 kdur2 = 0.012
 kdec2 = 0.07
 iolaps2 = 1700
 ifna2 = 16
 
 ifnb = 18
 itotdur = 800
 
 afof1 fof iAmp, kfreq+kjitter1, kform1, koct1, kband1, kris1, kdur1, kdec1, iolaps1, ifna1, ifnb, itotdur
 afof2 fof iAmp*0.9, (kfreq*2.01)+kjitter2, kform2, koct2, kband2, kris2, kdur2, kdec2, iolaps2, ifna2, ifnb, itotdur
 
 kEnv expseg 0.0001, p15, p16, p3-p15-p17, p16, p17, 0.0001

 asomma = (afof1 + afof2*0.72) * kEnv
 asomma dam asomma, 0.7, 1.2, 0.8, 0.001, 0.2
 asomma = asomma*2.9
 adelaytime randomi 0.01, 0.5, 4
 asommadel vdelay asomma, adelaytime, 0.6
 
 ipan = p18

 outch 1, asomma * (1-p18)
 outch 2, asommadel * p18
 
 garev1L = garev1L + asomma*0.3
 garev1R = garev1R + asommadel*0.3

 endin


instr 100 ; REV
 
 ktime1 randomi  2.3, 3.6, 6
 ktime2 randomi  2.2, 3.7, 7

 arev1L reverb2 garev1L, ktime1, 0.1
 arev1R reverb2 garev1R, ktime2, 0.8
 
 atimedelay1 randomi 0.3, 0.4, 5
 atimedelay2 randomi 0.3, 0.4, 8
 arev2L vdelay arev1L, atimedelay1, 0.6
 arev2R vdelay arev1R, atimedelay2, 0.6
 
 atotL = (arev1L*0.12) + (arev2R*0.34)
 atotR = (arev1R*0.12) + (arev2L*0.34)
 
 krandom1 randomi 0.25, 0.41, 8
 krandom2 randomi 0.23, 0.39, 7
 
 outch 1, atotL*krandom1
 outch 2, atotR*krandom2
 
 clear garev1L
 clear garev1R 
 
 endin



</CsInstruments>
<CsScore>


t 0 60 20 46 20 35; TEMPO

; function - "buzz1";
f1 0 2 -2 3 4
f2 0 4097 10 1 /*contents of f2 dont matter */
f3 0 4096 10 1
f4 0 4097 10 1 0 1 0 1 0 1 0 1 1 1

; function - buzz2
f6 0 4096 10 1 0.5 0.9


; functions - fof1
f 15 0 4096 10 1 0 1 0 1 0 1   
f 16 0 4096 10 1 0 1 0 0  1    
; sigmoid wave
f 18 0 1024 19 0.5 0.5 270 0.5 


;REVERB
i 100 0 53;


;                 dens    dens  form   form      vol      pan      REV     rAttack Decay
i "buzz" 0   5    2      120  1200    200       0.1      0.11     0.1       0.2    2
i "buzz" 2   8    1      34   450     12200     0.11     0.6       0.3      0.01   2
i "buzz" 4   6    20     5    500     4600      0.12     0.1       0.5      0.1    2
i "buzz" 8   9    43     680  15500   300      0.092    0.4       0.01      0.1    2


i "buzz" 19  15    4     120   14200   200       0.12    0.15      0.9      0.01   3
i "buzz" 21  13    3.8   35    2000    14200     0.09    0.49      0.3      0.01   3
   

i "buzz" 35  2    611    2  4000    4012      0.013     0.9       0.81      0.1   3
i "buzz" 35  2    40     43   1000   112       0.02      0.1       0.1      0.1   3

i "buzz" 39 2     20     27   3000    300      0.4       0.4       0.3      0.4   3
i "buzz" 39 2      4     72   79      5112     0.14      0.1       0.3      0.8   3
   
i "buzz" 42  6    11     49   50      1112      0.1      0.3       0.1      2.5   2
i "buzz" 42  9     2     812  100     4112     0.12      0.7       0.45     3.7  4
 

;                  Pitch1   Pitch2  Pitch3   Pitch4 -  portamenti(somma = 1)-Form1  Band1  Form2  Band2  Att    VOl  Rel   Pan
i "fof" 0     13   4.01    6.01    4.01     3.05       0.15   0.35  0.50     500   200     890    59    7.31   0.21  4.3  0.2
i "fof" 0     14   4.09    3.08    5.03     4.11       0.35   0.50  0.25     600   200     700    59    6.31   0.21  5.3  0.99


i "fof" 50   10    5.09    6.08    7.08     4.08      0.40   0.20  0.40      6230  720      2700   85   4.01   0.02 3.03  0.01
i "fof" 50.05 10   5.09    6.08    7.08     4.08      0.40   0.20  0.40      4230  420      1700   15   4.01   0.03 4.03  0.67

i "fof" 51    10   5.09    6.03    7.02     4.11      0.40   0.20  0.40      2300   720     5700   859  4.01   0.01 3.03  0.78
i "fof" 51.03 10   5.09    6.03    7.02     4.11      0.40   0.20  0.40      2300   720     5700   859  4.6    0.01 3.03  0.23

e 2



</CsScore>
</CsoundSynthesizer>
