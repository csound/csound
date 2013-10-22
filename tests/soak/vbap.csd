<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vbap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4	;quad
0dbfs  = 1

vbaplsinit 2, 4, 0, 90, 180, 270

instr 1

asig diskin2 "beats.wav", 1, 0, 1			;loop beats.wav
kaz  line 0, p3, p4					;come from right rear speaker &	
a1,a2,a3,a4 vbap  asig, 180, 100, kaz			;change spread of soundsource
     printks "spread of source = %d\n", 1, kaz		;print spread value
     outq a1,a2,a3,a4

endin 
</CsInstruments>
<CsScore>

i 1 0 12 100

e
</CsScore>
</CsoundSynthesizer>
