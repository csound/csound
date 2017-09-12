<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscil1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giPan ftgen 0, 0, 8, -2, .5, .2, .8, .1, .9, 0, 1, .5

instr     1   

istay = 2 ;how many seconds to stay on the first table value
asig   vco2 .3, 220
kpan   oscil1 istay, 1, p3-istay, giPan ;create panning 
       printk2 kpan ;print when new value
aL, aR pan2 asig, kpan ;apply panning
       outs aL, aR

endin
</CsInstruments>
<CsScore>                                                                                  
i 1  0  10
e
</CsScore>
</CsoundSynthesizer>
