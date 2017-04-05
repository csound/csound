<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pow.wav        ; output to audio file
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
    
instr 1	; Lo-Fi sound

kpow	  = 10						;exponent
kbase	  line 1, p3, 1.4				;vary the base
kQuantize pow kbase, kpow
kQuantize = kQuantize*0.5				;half the number of steps for each side of a bipolar signal
printk2	  kQuantize
asig	  diskin2 "fox.wav", 1, 0, 1			;loop the fox
asig	  = round(asig * kQuantize) / kQuantize		;quantize and scale audio signal
    	  outs asig, asig  

endin
</CsInstruments>
<CsScore>

i1 0 19.2

e
</CsScore>
</CsoundSynthesizer>

