<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelayxws.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ims  =	.5						;maximum delay time in seconds
iws  =	512						;window size
adl  =	.5						;delay time
asig1, asig2 diskin2 "drumsSlp.wav", 1, 0, 1		;loop stereo file drumsSlp.wav
a2   poscil .2, .25  					;make an LFO, 4 cycle per 10 seconds
adl  = a2 + ims/2      					;offset the LFO so that it is positive
aoutL, aoutR vdelayxws asig1, asig2, adl, ims, iws	;use the LFO to control delay time
     outs aoutL * .8, aoutR * .8

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 11

e
</CsScore>
</CsoundSynthesizer> 
