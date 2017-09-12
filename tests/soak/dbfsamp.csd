<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dbfsamp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

iamp = p4
idb  = dbfsamp(iamp)
     print idb
asig vco2 iamp, 110	;sawtooth
     outs asig, asig

endin

</CsInstruments>
<CsScore>

i 1 0 1 1
i 1 + 1 100
i 1 + 1 1000
i 1 + 1 10000
i 1 + 1 30000
e

</CsScore>
</CsoundSynthesizer>
