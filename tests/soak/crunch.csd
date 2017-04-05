<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o crunch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

asig   crunch 0.8, 0.1, 7, p4
       outs asig, asig

endin

</CsInstruments>
<CsScore>

i1 0 1 .9
i1 1 1 .1

e
</CsScore>
</CsoundSynthesizer>
