<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform 
-odac     ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too 
; For Non-realtime ouput leave only the line below: 
; -o grain.wav -W ;;; for file output any platform 
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1 

insnd   = 10 
ibasfrq = 44100 / ftlen(insnd) ; Use original sample rate of insnd file 

kamp   expseg .001, p3/2, .1, p3/2, .01 ;a swell in amplitude
kpitch line ibasfrq, p3, ibasfrq * .8 
kdens  line 600, p3, 100 
kaoff  line 0, p3, .1
kpoff  line 0, p3, ibasfrq * .5 
kgdur  line .4, p3, .01
imaxgdur =  .5 

asigL  grain kamp, kpitch, kdens, kaoff, kpoff, kgdur, insnd, 5, imaxgdur, 0.0 
asigR  grain kamp, kpitch, kdens, kaoff, kpoff, kgdur, insnd, 5, imaxgdur, 0.0 
       outs asigL, asigR

endin 
</CsInstruments> 
<CsScore> 
f5  0 512  20 2 ; Hanning window 
f10 0 16384 1  "drumsMlp.wav" 0 0 0 

i1 0 15 
e 
</CsScore> 
</CsoundSynthesizer> 