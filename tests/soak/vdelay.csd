<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vdelay.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr   = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

ims  = 100				;maximum delay time in msec
aout poscil .8, 220, 1			;make a signal
a2   poscil3 ims/2, 1/p3, 1		;make an LFO
a2   = a2 + ims/2 			;offset the LFO so that it is positive
asig vdelay aout, a2, ims		;use the LFO to control delay time
     outs  asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1 ;sine wave

i 1 0 5 

e
</CsScore>
</CsoundSynthesizer> 

