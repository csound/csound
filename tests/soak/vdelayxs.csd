<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelayxs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ims  =	.5						;maximum delay time in seconds
iws  =	1024						;window size
adl  =	.5						;delay time
asig1, asig2 diskin2 "kickroll.wav", 1, 0, 1		;loop stereo file kickroll.wav
a2   poscil .25, .1, 1					;make an LFO, 1 cycle per 2 seconds
adl  = a2 + ims/2      					;offset the LFO so that it is positive
aoutL, aoutR vdelayxs asig1, asig2, adl, ims, iws	;use the LFO to control delay time
     outs aoutL, aoutR

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 10

e
</CsScore>
</CsoundSynthesizer> 
