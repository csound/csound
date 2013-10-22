<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o linen.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; p4=amp
; p5=freq
; p6=attack time
; p7=release time
ares linen  p4, p6, p3, p7 
asig poscil ares, p5, 1    
     outs   asig, asig     
                                         
endin
</CsInstruments>
<CsScore>
f1   0    4096 10 1      ; sine wave

;ins strt dur amp  freq attack release
i1   0    1   .5   440   0.5    0.7
i1   1.5  1   .2   440   0.9    0.1
i1   3    1   .2   880   0.02   0.99
i1   4.5  1   .2   880   0.7    0.01
i1   6    3   .7   220   0.5    0.5
e
</CsScore>
</CsoundSynthesizer>
