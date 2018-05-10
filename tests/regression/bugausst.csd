<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
 kcps = 440
a1 gausstrig 0.2, kcps, 0.1, 0, 1
outs  a1, a1

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1
i1 0 16
e
</CsScore>
</CsoundSynthesizer>
