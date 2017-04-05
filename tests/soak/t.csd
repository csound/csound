<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o t.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

aenv expseg .01, p3*0.25, 1, p3*0.75, 0.01
asig poscil3 .8*aenv, p4, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave

t 0 240 12 30 15 240	;start tempo = 240 
		
i 1 0 2 110		;tempo = 240
i 1 3 2 220		;slow down &
i 1 6 2 440		;slow down &
i 1 9 2 880		;slow down &		
i 1 12 2 110		;slow down to 30 at 12 seconds
i 1 15 2 220		;speed up to 240 again
i 1 18 2 440		;stay at tempo 240
i 1 21 2 880
e
</CsScore>
</CsoundSynthesizer>

