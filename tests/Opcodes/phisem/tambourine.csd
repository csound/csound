<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tambourine.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idamp = p4
asig  tambourine .8, 0.01, 30, idamp, 0.4
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 .2 0
i 1 + .2 >
i 1 + 1 .7
e
</CsScore>
</CsoundSynthesizer>
