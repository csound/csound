<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mandol.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kamp    = p4
ksize   = p5
kdetune = p6
asig mandol kamp, 880, .4, kdetune, 0.99, ksize, 1, 220
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; "mandpluk.aiff" audio file
f 1 0 8192 1 "mandpluk.aiff" 0 0 0

i 1 .5 1  1  2 .99
i 1 +  1 .5  1 .99	;lower volume to compensate
i 1 +  3 .3 .3 .99	;lower volume to compensate

i 1 4  1  1  2 .39	;change detune value
i 1 +  1 .5  1 .39
i 1 +  3 .3 .3 .39
e
</CsScore>
</CsoundSynthesizer>
