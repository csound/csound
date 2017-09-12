<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sekere.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idamp = p4			;vary damping amount
asig  sekere 1, 0.01, 64, idamp
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 1 .1
i1 + 1 .9
e
</CsScore>
</CsoundSynthesizer>
