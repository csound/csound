<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac    ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o tanh.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig1 vco  1, 440, 2, 0.4, 1
asig2 vco  1, 800, 3, 0.5, 1
asig  =    asig1+asig2              ; will go out of range
outs  tanh(asig), tanh(asig)        ; but tanh is a limiter

endin

</CsInstruments>
<CsScore>
f1 0 65536 10 1 ; sine

i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
