<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o timeinsts.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

kvib init 1
ktim timeinsts				;read time 

if ktim > 2 then			;do something after 2 seconds
   kvib oscili 2, 3, giSine		;make a vibrato
endif

asig poscil .5, 600+kvib, giSine	;add vibrato
     outs asig, asig

endin 
</CsInstruments>
<CsScore>

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
