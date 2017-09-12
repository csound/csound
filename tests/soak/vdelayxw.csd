<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelayxw.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ims  =	.5				;maximum delay time in seconds
iws  =	1024				;best quality
adl  =	.5				;delay time
asig diskin2 "flute.aiff", .5, 0, 1	;loop flute.aiff at half speed
a2   poscil3 .2, .1, 1			;make an LFO, 1 cycle per 2 seconds
adl  = a2 + ims/2      			;offset the LFO so that it is positive
aout vdelayxw asig, adl, ims, iws	;use the LFO to control delay time
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 10

e
</CsScore>
</CsoundSynthesizer> 
