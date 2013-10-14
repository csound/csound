<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform 
-odac     ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too 
; For Non-realtime ouput leave only the line below: 
; -o gen20.wav -W ;;; for file output any platform 
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1 

insnd   = 10 				;"fox.wav"
ibasfrq = 44100 / ftlen(insnd)		;use original sample rate of insnd file 

kamp   expseg .001, p3/2, .7, p3/2, .8	;envelope
kpitch line ibasfrq, p3, ibasfrq * .8 
kdens  line 600, p3, 10 
kaoff  line 0, p3, .1
kpoff  line 0, p3, ibasfrq * .5 
kgdur  line .04, p3, .001		;shorten duration of grain during note
imaxgdur =  .5 
igfn = p4 				;different windows
asigL  grain kamp, kpitch, kdens, kaoff, kpoff, kgdur, insnd, igfn, imaxgdur, 0.0 
asigR  grain kamp, kpitch, kdens, kaoff, kpoff, kgdur, insnd, igfn, imaxgdur, 0.0 
       outs asigL, asigR

endin 
</CsInstruments> 
<CsScore> 
f1  0 512  20 2		;Hanning window 
f2  0 512  20 6 1	;Gaussian window 
f10 0 16384 1 "fox.wav" 0 0 0 

i1 0 5 1		;use Hanning window 
i1 + 5 2		;use Gaussian window
e 
</CsScore> 
</CsoundSynthesizer> 

