<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o guiro.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

a1  guiro .8, p4
    outs a1, a1

endin
</CsInstruments>
<CsScore>

i1 0 1  1
i1 + 1 .01
e
</CsScore>
</CsoundSynthesizer>
