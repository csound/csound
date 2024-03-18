<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o bamboo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1
asig  bamboo p4, 0.01,0, 0, 0, 8000
      outs asig, asig

endin

</CsInstruments>
<CsScore>

i1 0 1 20000
i1 2 1 20000
e

</CsScore>
</CsoundSynthesizer>
