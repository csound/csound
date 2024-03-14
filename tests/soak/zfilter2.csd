<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o zfilter2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By Stefano Cucchi 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	
 
kdamp linseg p5, p3, p6 
kfreq linseg p7, p3, p8				                                            
kcps  init cpspch(p4)
asig1 vco2 .2, kcps			
asaw zfilter2 asig1, kdamp, kfreq, 2, 2, 0.81, 0.713, 0.0001, 0.99	

acompare oscil 0.2, 440 ; signal comparator for volume 
asaw balance asaw, acompare ; adjust the volume. Volume can have big variation due to filter.
      outs asaw, asaw

endin
</CsInstruments>
<CsScore>

i 1 0 5 6.00 0.6 0.99 0.006 0.006 ; varying ringing time (from 0.6 to 0.99)
i 1 6 5 6.00 0.3 0.3 0.01 -0.8    ; varying frequency warp factor (from 0.01 to -0.8)
e
</CsScore>
</CsoundSynthesizer>
