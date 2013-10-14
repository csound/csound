<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o maxalloc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

maxalloc 1, 3	; Limit to three instances.
 
instr 1

asig oscil .3, p4, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; sine
f 1 0 32768 10 1

i 1 0 5 220	;1
i 1 1 4 440	;2
i 1 2 3 880	;3, limit is reached
i 1 3 2 1320	;is not played
i 1 4 1 1760	;is not played
e
</CsScore>
</CsoundSynthesizer>
