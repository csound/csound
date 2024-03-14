<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o a.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

aenv expseg .01, p3*0.25, 1, p3*0.75, 0.01
asig poscil3 .8*aenv, p4
     outs asig, asig

endin
</CsInstruments>
<CsScore>
;two sections
s
a 0 0 6		;advance score 6 seconds
i 1 0 2 110	;these first 2 notes
i 1 3 2 220	;will not sound
i 1 6 2 440
i 1 9 2 880
s
a 0 3 6		;advance score 6 seconds, but do this after 3 seconds
i 1 0 2 110	;this will sound, because action time (p2) from a statement = 3
i 1 3 2 220	;so these 2 notes
i 1 6 2 440	;will not sound
i 1 9 2 880	;and this one will
e
</CsScore>
</CsoundSynthesizer>

