<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o scoreline.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ktrig metro 3				;trigger 3 times a second
scoreline {{				;so it sounds like an echo
            i 2  0  3  "flute.aiff"
            i 2  1  3  "beats.wav"
            }}, ktrig
ktrig = 0
        
endin

instr 2

asig soundin p4
     outs asig*.3, asig*.3

endin
</CsInstruments>
<CsScore>

i1 0 2	;play for 2 seconds, so the samples are played 6 times 
e
</CsScore>
</CsoundSynthesizer>
