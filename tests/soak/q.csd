<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o q.wav -W ;;; for file output any platform
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
f 1 0 16384 10 1;sine wave
s
q 1 6 0		;mute at 6 seconds in this section
i 1 0 2 110
i 1 3 2 220
i 1 6 2 440
i 1 9 2 880

s
q 1 6 1		;unmute again at 6 seconds in this section
i 1 0 2 110
i 1 3 2 220
i 1 6 2 440
i 1 9 2 880
e
</CsScore>
</CsoundSynthesizer>

