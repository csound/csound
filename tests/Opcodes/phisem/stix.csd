<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o stix.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

idamp = p4			;vary damping amount
asig stix .5, 0.01, 30, idamp
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 1 .3
i1 + 1  >
i1 + 1  >
i1 + 1 .95

e
</CsScore>
</CsoundSynthesizer>
