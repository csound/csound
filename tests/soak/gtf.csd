<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gtf.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	 
                                        
kcps  init cpspch(p4) 
asig1 vco2 0.5, kcps ; SOUND

kfreq1 linseg p5, p3, p6 ; frequency filter 1
kfreq2 expseg p6, p3, p5 ; frequency filter 2
idecay = p7 ; keep it very small

afilter1 gtf asig1, kfreq1, idecay ; SOUND - filter 1
afilter2 gtf asig1, kfreq2, idecay ; SOUND - filter 2

aref oscili 0.25, 440 ; AMPLITUDE reference
afilter1 balance afilter1, aref ; compare filtered SOUND with reference
afilter2 balance afilter2, aref ; ; compare filtered SOUND with reference

outs afilter1, afilter2

endin

</CsInstruments>


<CsScore>

i 1 0 5 6.00 200 12000 0.1
i 1 5 5 6.00 200 12000 0.01		


e
</CsScore>
</CsoundSynthesizer>
