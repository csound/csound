<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o setscorepos.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 8192, 10, 1

instr 1

asig poscil 0.5, p4, giSine		;play something
     outs asig, asig
endin

instr 11

setscorepos 8.5				
endin

</CsInstruments>
<CsScore>

i1 0 2 220	;this one will be played
i11 2.5 1	;start setscorepos now
i1 3 2 330	;skip this note
i1 6 2 440	;and this one
i1 9 2 550	;play this one        

</CsScore>
</CsoundSynthesizer>
