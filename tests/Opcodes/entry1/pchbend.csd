<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Q1 -Ma  ;;;realtime audio out and midi in (on all inputs) and out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pchbend.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments> 

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr   1	;use external midi keyboard

icps cpsmidi
kbnd pchbend 0, 100					;one octave lower and higher
kenv linsegr 0,.001, 1, .1, 0				;amplitude envelope 
asig pluck .8 * kenv, icps+kbnd, 440, 0, 1
     outs asig, asig

endin 
</CsInstruments> 
<CsScore> 

f 0 30	;runs 30 seconds
 
</CsScore> 
</CsoundSynthesizer>   
