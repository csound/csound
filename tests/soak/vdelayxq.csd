<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelayxq.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

instr 1 

ims   =	.5						;maximum delay time in seconds
iws   =	1024						;window size
adl   =	.5
aout1  diskin2 "beats.wav", 1, 0, 1			;loop beats.wav
aout2  diskin2 "fox.wav", 1, 0, 1			;loop fox.wav
aout3  diskin2 "Church.wav", 1, 0, 1			;loop Church.wav
aout4  diskin2 "flute.aiff", 1, 0, 1			;loop flute.aiff
a2    poscil .1, .5, 1					;make an LFO, 1 cycle per 2 seconds
adl   = a2 + ims/2      				;offset the LFO so that it is positive
aout1, aout2, aout3, aout4 vdelayxq aout1, aout2, aout3, aout4, adl, ims, iws; Use the LFO to control delay time
      outq aout1, aout2, aout3, aout4

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1

i 1 0 10 

e
</CsScore>
</CsoundSynthesizer> 
