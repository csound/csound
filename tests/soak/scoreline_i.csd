<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac --old-parser   ;;;realtime audio out
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

scoreline_i {{
            i 2  0  3  "flute.aiff"
            i 2  1  3  "beats.wav"
            }}
        
endin

instr 2

asig soundin p4
     outs asig*.8, asig*.8

endin
</CsInstruments>
<CsScore>

i1 0 1
e
</CsScore>
</CsoundSynthesizer>
