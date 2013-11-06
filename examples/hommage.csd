<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>

sr= 44100
kr = 4410
ksmps = 10
nchnls= 2
0dbfs = 1

instr 1

kfreq    expon      p5, p3, p6
kinterval expon      p7, p3, p8    
kglissdur line     p9, p3, p10

idir = p11
iphs = p12
iwin = p13


kph phasor idir/kglissdur, iphs

kamp1   tablei kph, iwin, 1, 0, 1
kpa1    tablei kph, 3, 1, 0, 1
kifreq1  = kfreq                                     ; plays base_freq 
aout1   oscili  kamp1,kifreq1,1                      ; 
aout1l = kpa1*aout1
aout1r = -(kpa1-1)*aout1

kamp2   tablei kph, iwin, 1, .1, 1  
kpa2    tablei kph, 3, 1, .1, 1
kifreq2  = kifreq1*kinterval                           ; plays base_freq * interval
aout2   oscili  kamp2,kifreq2,1 
aout2l = kpa2*aout2
aout2r = -(kpa2-1)*aout2


kamp3   tablei kph, iwin, 1, .2, 1                        ; 2/10 table offset
kpa3    tablei kph, 3, 1, .2, 1
kifreq3  = kifreq2*kinterval                           ; plays base_freq * interval^2
aout3   oscili  kamp3,kifreq3,1
aout3l = kpa3*aout3
aout3r = -(kpa3-1)*aout3



kamp4   tablei kph, iwin, 1, .3, 1
kpa4    tablei kph, 3, 1, .3, 1
kifreq4  = kifreq3*kinterval  
aout4   oscili  kamp4,kifreq4,1
aout4l = kpa4*aout4
aout4r = -(kpa4-1)*aout4


kamp5   tablei kph, iwin, 1, .4, 1
kpa5    tablei kph, 3, 1, .4, 1
kifreq5  = kifreq4*kinterval 
aout5   oscili  kamp5,kifreq5,1
aout5l = kpa5*aout5
aout5r = -(kpa5-1)*aout5

kamp6   tablei kph, iwin, 1, .5, 1
kpa6    tablei kph, 3, 1, .5, 1
kifreq6  = kifreq5*kinterval  
aout6   oscili  kamp6,kifreq6,1
aout6l = kpa6*aout6
aout6r = -(kpa6-1)*aout6

kamp7   tablei kph, iwin, 1, .6, 1
kpa7    tablei kph, 3, 1, .6, 1
kifreq7  = kifreq6*kinterval   
aout7   oscili  kamp7,kifreq7,1
aout7l = kpa7*aout7
aout7r = -(kpa7-1)*aout7

kamp8   tablei kph, iwin, 1, .7, 1
kpa8    tablei kph, 3, 1, .7, 1
kifreq8  = kifreq7*kinterval   
aout8   oscili  kamp8,kifreq8,1
aout8l = kpa8*aout8
aout8r = -(kpa8-1)*aout8

kamp9   tablei kph, iwin, 1, .8, 1
kpa9    tablei kph, 3, 1, .8, 1 
kifreq9  = kifreq8*kinterval   
aout9   oscili  kamp9,kifreq9,1
aout9l = kpa9*aout9
aout9r = -(kpa9-1)*aout9

kamp10   tablei kph, iwin, 1, .9, 1  
kpa10    tablei kph, 3, 1, .9, 1
kifreq10  = kifreq9*kinterval  
aout10   oscili  kamp10,kifreq10,1               
aout10l = kpa10*aout10
aout10r = -(kpa10-1)*aout10


kline linen ampdb(p4)*0dbfs, p14, p3, p14

;kpleft = (sqrt(2)/2)*(1-kpan/sqrt(1+(kpan*kpan)))
;kpright = (sqrt(2)/2)* (1+kpan/sqrt(1+(kpan*kpan)))


aright = aout1r+aout2r+aout3r+aout4r+aout5r+aout6r+aout7r+aout8r+aout9r+aout10r 
aleft =  aout1l+aout2l+aout3l+aout4l+aout5l+aout6l+aout7l+aout8l+aout9l+aout10l    
     outs    aleft*kline, aright*kline

endin


instr 2


kfreq    expon      p5, p3, p6
kinterval expon      p7, p3, p8    
kglissdur line     p9, p3, p10
idir = p11
iphs = p12
iwin = p13
iq = p15
isrc = p16


noise: asig rand 1

next:
kph phasor idir/kglissdur, iphs

kamp1   tablei kph, iwin, 1, 0, 1
kpa1    tablei kph, 3, 1, 0, 1
kifreq1  = kfreq                                     ; plays base_freq 
aout1   reson asig,kifreq1,kifreq1/iq,2 
aout1 = kamp1*aout1                  
aout1l = kpa1*aout1
aout1r = -(kpa1-1)*aout1

kamp2   tablei kph, iwin, 1, .1, 1  
kpa2    tablei kph, 3, 1, .1, 1
kifreq2  = kifreq1*kinterval                           ; plays base_freq * interval
aout2   reson asig,kifreq2,kifreq2/iq,2 
aout2 = kamp2*aout2 
aout2l = kpa2*aout2
aout2r = -(kpa2-1)*aout2


kamp3   tablei kph, iwin, 1, .2, 1                        ; 2/10 table offset
kpa3    tablei kph, 3, 1, .2, 1
kifreq3  = kifreq2*kinterval                           ; plays base_freq * interval^2
aout3   reson asig,kifreq3,kifreq3/iq,2 
aout3 = kamp3*aout3 
aout3l = kpa3*aout3
aout3r = -(kpa3-1)*aout3



kamp4   tablei kph, iwin, 1, .3, 1
kpa4    tablei kph, 3, 1, .3, 1
kifreq4  = kifreq3*kinterval  
aout4   reson asig,kifreq4,kifreq4/iq,2 
aout4 = kamp4*aout4 
aout4l = kpa4*aout4
aout4r = -(kpa4-1)*aout4


kamp5   tablei kph, iwin, 1, .4, 1
kpa5    tablei kph, 3, 1, .4, 1
kifreq5  = kifreq4*kinterval 
aout5   reson asig,kifreq5,kifreq5/iq,2 
aout5 = kamp5*aout5 
aout5l = kpa5*aout5
aout5r = -(kpa5-1)*aout5

kamp6   tablei kph, iwin, 1, .5, 1
kpa6    tablei kph, 3, 1, .5, 1
kifreq6  = kifreq5*kinterval  
aout6   reson asig,kifreq6,kifreq6/iq,2 
aout6 = kamp6*aout6 
aout6l = kpa6*aout6
aout6r = -(kpa6-1)*aout6

kamp7   tablei kph, iwin, 1, .6, 1
kpa7    tablei kph, 3, 1, .6, 1
kifreq7  = kifreq6*kinterval   
aout7   reson asig,kifreq7,kifreq7/iq,2 
aout7 = kamp7*aout7 
aout7l = kpa7*aout7
aout7r = -(kpa7-1)*aout7

kamp8   tablei kph, iwin, 1, .7, 1
kpa8    tablei kph, 3, 1, .7, 1
kifreq8  = kifreq7*kinterval   
aout8   reson asig,kifreq8,kifreq8/iq,2 
aout8 = kamp8*aout8 
aout8l = kpa8*aout8
aout8r = -(kpa8-1)*aout8

kamp9   tablei kph, iwin, 1, .8, 1
kpa9    tablei kph, 3, 1, .8, 1 
kifreq9  = kifreq8*kinterval   
aout9   reson asig,kifreq9,kifreq9/iq,2 
aout9 = kamp9*aout9
aout9l = kpa9*aout9
aout9r = -(kpa9-1)*aout9

kamp10   tablei kph, iwin, 1, .9, 1  
kpa10    tablei kph, 3, 1, .9, 1
kifreq10  = kifreq9*kinterval  
aout10   reson asig,kifreq10,kifreq10/iq,2 
aout10 = kamp10*aout10              
aout10l = kpa10*aout10
aout10r = -(kpa10-1)*aout10


kline linen ampdb(p4)*0dbfs, p14, p3, p14

;kpleft = (sqrt(2)/2)*(1-kpan/sqrt(1+(kpan*kpan)))
;kpright = (sqrt(2)/2)* (1+kpan/sqrt(1+(kpan*kpan)))


aright = aout1r+aout2r+aout3r+aout4r+aout5r+aout6r+aout7r+aout8r+aout9r+aout10r 
aleft =  aout1l+aout2l+aout3l+aout4l+aout5l+aout6l+aout7l+aout8l+aout9l+aout10l    
     outs    aleft*kline, aright*kline

endin


instr 3


kfreq0    expon      p5, p3, p6
kinterval expon      p7, p3, p8    
kglissdur line     p9, p3, p10
koct line  p16, p3, p17
idir = p11
iphs = p12
iwin = p13
iq  = p15
idur = .04
iris = .007
idec = .003
iolaps = 1000

kph phasor idir/kglissdur, iphs
kran randi kfreq0*0.005, 4
kfreq = kfreq0+kran

kamp1   tablei kph, iwin, 1, 0, 1
kpa1    tablei kph, 3, 1, 0, 1
kifreq1  = kfreq0*kinterval                                    
aout1   fof  kamp1, kfreq, kifreq1, koct, kifreq1/iq, iris, idur, idec, iolaps, 1, 4, p3
aout1l = kpa1*aout1
aout1r = -(kpa1-1)*aout1

kamp2   tablei kph, iwin, 1, .1, 1  
kpa2    tablei kph, 3, 1, .1, 1
kifreq2  = kifreq1*kinterval                           ; plays base_freq * interval
aout2  fof  kamp2, kfreq, kifreq2, koct, kifreq2/iq, iris, idur, idec, iolaps, 1, 4, p3
aout2l = kpa2*aout2
aout2r = -(kpa2-1)*aout2


kamp3   tablei kph, iwin, 1, .2, 1                        ; 2/10 table offset
kpa3    tablei kph, 3, 1, .2, 1
kifreq3  = kifreq2*kinterval                           ; plays base_freq * interval^2
aout3   fof  kamp3,kfreq, kifreq3, koct, kifreq3/iq, iris, idur, idec, iolaps, 1, 4, p3
aout3l = kpa3*aout3
aout3r = -(kpa3-1)*aout3



kamp4   tablei kph, iwin, 1, .3, 1
kpa4    tablei kph, 3, 1, .3, 1
kifreq4  = kifreq3*kinterval  
aout4   fof  kamp4,kfreq, kifreq4, koct, kifreq4/iq, iris, idur, idec, iolaps, 1, 4, p3
aout4l = kpa4*aout4
aout4r = -(kpa4-1)*aout4


kamp5   tablei kph, iwin, 1, .4, 1
kpa5    tablei kph, 3, 1, .4, 1
kifreq5  = kifreq4*kinterval 
aout5   fof  kamp5,kfreq, kifreq5, koct, kifreq5/iq, iris, idur, idec, iolaps, 1, 4, p3
aout5l = kpa5*aout5
aout5r = -(kpa5-1)*aout5

kamp6   tablei kph, iwin, 1, .5, 1
kpa6    tablei kph, 3, 1, .5, 1
kifreq6  = kifreq5*kinterval  
aout6   fof  kamp6,kfreq, kifreq6, koct, kifreq6/iq, iris, idur, idec, iolaps, 1, 4, p3
aout6l = kpa6*aout6
aout6r = -(kpa6-1)*aout6

kamp7   tablei kph, iwin, 1, .6, 1
kpa7    tablei kph, 3, 1, .6, 1
kifreq7  = kifreq6*kinterval   
aout7   fof  kamp7,kfreq, kifreq7, koct, kifreq7/iq, iris, idur, idec, iolaps, 1, 4, p3
aout7l = kpa7*aout7
aout7r = -(kpa7-1)*aout7

kamp8   tablei kph, iwin, 1, .7, 1
kpa8    tablei kph, 3, 1, .7, 1
kifreq8  = kifreq7*kinterval   
aout8  fof kamp8,kfreq, kifreq8, koct, kifreq8/iq, iris, idur, idec, iolaps, 1, 4, p3
aout8l = kpa8*aout8
aout8r = -(kpa8-1)*aout8

kamp9   tablei kph, iwin, 1, .8, 1
kpa9    tablei kph, 3, 1, .8, 1 
kifreq9  = kifreq8*kinterval   
aout9   fof  kamp9,kfreq, kifreq9, koct, kifreq9/iq, iris, idur, idec, iolaps, 1, 4, p3
aout9l = kpa9*aout9
aout9r = -(kpa9-1)*aout9

kamp10   tablei kph, iwin, 1, .9, 1  
kpa10    tablei kph, 3, 1, .9, 1
kifreq10  = kifreq9*kinterval  
aout10   fof  kamp10,kfreq, kifreq10, koct, kifreq10/iq, iris, idur, idec, iolaps, 1, 4, p3             
aout10l = kpa10*aout10
aout10r = -(kpa10-1)*aout10



kline linen ampdb(p4)*0dbfs*.6, p14, p3, p14

;kpleft = (sqrt(2)/2)*(1-kpan/sqrt(1+(kpan*kpan)))
;kpright = (sqrt(2)/2)* (1+kpan/sqrt(1+(kpan*kpan)))


aright = aout1r+aout2r+aout3r+aout4r+aout5r+aout6r+aout7r+aout8r+aout9r+aout10r 
aleft =  aout1l+aout2l+aout3l+aout4l+aout5l+aout6l+aout7l+aout8l+aout9l+aout10l    
     outs    aleft*kline, aright*kline

endin

</CsInstruments>

<CsScore>

; senoide
f1 0 4096 10 1 ;0 .8 0 .4 0 .6 0 .2
f4 0 4097 5  .000001 4096 1

f21 0 4096 20 5 ;blackman
f22 0 4096 20 6 ;gauss
f23 0 4096 20 3 ;triang

f3 0 512 7 0.00001 128 0.00001 256 1 128 1

;     dur amp      fr1 fr2   int1 int2 sp1 sp2 idir iphs iwin att/dec
i2 0.5  62  -12  1000 1100  1.3 1.1  18  .6   1  .75     22    2   500 1
i1 45   40  -16  1050  980  1.04 1.06  1   4  -1   .1     23    8   
i2 50   30   -13  800  500  1.03 1.5   .3  10  1   0     21     2  250 1
i1 65   36   -13  100   110  1.6  1.01  2   4   -1  .5    22    1
i2 75   25   -18   800  950  1.5 1.15   .9  2  -1  .7    23    9   50  1   
i1 87  20   -15   1200 1100  1.3 1.09 1   3   1   .25    21    2
i3 95  20   -13    110  130  1.55  1.2  4  2   -1  .5    23    3   20 0 0
i1 100  30   -18   5000 7000  1.004 1.09 2   1   -1  0    23    4
i2 105  20   -15   4000 4500  1.1   1.17  1.3  .8  1  0   21    6 100 1
i2 115  27   -24   2500 1000  1.1   1.3   5    8   -1 .75 22    4  50 1
i1 123  22   -18   2000 2050  1.008 1.02  .3   3   1  0   21    2  
i3 125   6   -20    800  850  1.1  1.3    6  6   -1  .5    22    3  500 0 0

</CsScore>

</CsoundSynthesizer>

