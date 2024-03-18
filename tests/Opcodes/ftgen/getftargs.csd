<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o getftargs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
Sargs getftargs 1, 1
puts Sargs, 1
endin

</CsInstruments>
<CsScore>
f 1 0 1024 "quadbezier" 0 0 0.5 200 0.8 450 0.33 600 0.1 800 0.4 1024 0
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
