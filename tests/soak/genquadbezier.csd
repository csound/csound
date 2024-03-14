<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o quadbezier.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
kndx phasor 1/p3
kenv tablei kndx, p4, 1
asig poscil kenv, 440, 1
     outs asig, asig
  
endin

</CsInstruments>
<CsScore>
f 1 0 32768 10 1
f 2 0 1024 "quadbezier" 0 140 0.61 324 0.53 338 0.27 449 0.32 571 0.08 675 0.5 873 0.47 1024 0
f 3 0 1024 "quadbezier" 0 92 0.04 94 0.25 177 0.58 373 0.39 537 0.15 675 0.5 910 0.68 1024 0
f 4 0 1024 "quadbezier" 0 196 0.68 537 0.71 873 0.7 1024 0

i 1 0 4 2
i 1 4 4 3
i 1 8 4 4
</CsScore>

</CsoundSynthesizer>
