<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vbaplsinit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 8	
0dbfs  = 1

vbaplsinit 2, 8, 0, 45, 90, 180, 270, 0, 0, 0		;5 speakers for 5.1 amps

instr 1

asig diskin2 "beats.wav", 1, 0, 1			;loop beats.wav
kazim line 1, p3, 355				
a1,a2,a3,a4,a5,a6,a7,a8 vbap8  asig, kazim, 0, 1	;change azimuth of soundsource
; Speaker mapping
aFL = a1 						; Front Left
aMF = a5 						; Mid Front 
aFR = a2 						; Front Right
aBL = a3 						; Back Left
aBR = a4 						; Back Right
    outo aFL,aFR,aBL,aBR,aMF,a6,a7,a8			;a6, a7 and a8 are dummies				

endin 
</CsInstruments>
<CsScore>

i 1 0 5

e
</CsScore>
</CsoundSynthesizer>
