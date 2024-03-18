<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rndseed.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

rndseed .15 ; change value for a different sequence
; Generate a random number sequence from -1000 to 1000 according to rndseed
instr 1	
  
kbin    =	birnd(1000)
printk2 kbin
asig    vco2    .5, 100
ares    rezzy   asig, 1500+kbin, 20
        outs ares, ares
endin

</CsInstruments>
<CsScore>
i 1 0 .3
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .
i 1 + .
e
</CsScore>
</CsoundSynthesizer>
