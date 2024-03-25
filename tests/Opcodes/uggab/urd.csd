<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o urd.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ktab  = 1			;ftable 1
kurd  = urd(ktab) 
ktrig metro 5			;triggers 5 times per second
kres  samphold kurd, ktrig	;sample and hold value of kurd
      printk2 kres		;print it
asig  poscil .5, 220+kres, 2
      outs asig, asig
endin

instr 2

seed 0	;every run new values

ktab  = 1 			;ftable 1
kurd  = urd(ktab) 
ktrig metro 5			;triggers 5 times per second
kres  samphold kurd, ktrig	;sample and hold value of kurd
      printk2 kres		;print it
asig  poscil .5, 220+kres, 2
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 -20 -42  10 20 .3 100 200 .7 ;30% choose between 10 and 20 and 70% between 100 and 200
f2 0 8192 10 1			  ;sine wave

i 1 0 5
i 2 6 5
e
</CsScore>
</CsoundSynthesizer>
