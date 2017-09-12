<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o v.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

aenv expseg .01, p3*0.25, 1, p3*0.75, 0.01
asig poscil3 .4*aenv, p4, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave
;because note 3 and 5 are played simultaneously and are nearly of the same frequency,
;played together they will create a "beating" sound.

i 1 0 2  110	; note1
v2
i 1 3 .  220	; note2
i 1 6 .  110	; note3
v1
i 1 9 .  880	; note4
i 1 12 . 100	; note5
e
</CsScore>
</CsoundSynthesizer>

