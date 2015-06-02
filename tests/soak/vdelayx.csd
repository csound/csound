<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelayx.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ims  =	.5				;maximum delay time in seconds
iws  =	1024				;window size
adl  =	.5				;delay time
asig diskin2 "fox.wav", 1, 0, 1		;loop fox.wav
a2   poscil .2, .2, 1			;make an LFO
adl  = a2 + ims/2      			;offset the LFO so that it is positive
aout vdelayx asig, adl, ims, iws	;use the LFO to control delay time
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 10

e
</CsScore>
</CsoundSynthesizer> 
