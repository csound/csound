<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out  
-odac           ;;;RT audio 
; For Non-realtime ouput leave only the line below:
; -o partikkel-panlaws.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

nchnls 	= 4
0dbfs	= 1

giSine ftgen 0, 0, 65536, 10, 1
giCosine ftgen 0, 0, 8192, 9, 1, 1, 90
giSigmoRise ftgen 0, 0, 8193, 19, 0.5, 1, 270, 1
giSigmoFall ftgen 0, 0, 8193, 19, 0.5, 1, 90, 1
giLinUp ftgen 0, 0, 8192, 7, 0, 8192, 1
giConcaveUp ftgen 0, 0, 8192, 16, 0, 8192, 1.5, 1
giConvexUp ftgen 0, 0, 8192, 16, 0, 8192, -1.5, 1
giPanLaws ftgen 0, 0, 8192, 18, 
    giLinUp, 1, 0, 1023, 
    giConcaveUp, 1, 1024, 2047,  
    giConvexUp, 1, 2048, 3071,  
    giSigmoRise, 1, 3072, 4095

instr 1

; channel masking table, using just one single mask here
ichannelmasks	ftgentmp	0, 0, 32, -2,  0, 0,   0

; continuously write to masking table, 
; slowly panning the grains from output to output 
; over a 10 second period
kchn phasor 0.1
kchn = kchn*4
tablew kchn, 2, ichannelmasks

; init unused arate signals
awavfm = 0
asamplepos1	= 0
async = 0

a1,a2,a3,a4 partikkel 4, 0, -1, async, 0, -1, giSigmoRise, giSigmoFall, 
    0.9, 0.5, 100, ampdbfs(-9), -1, 1, 0, -1, -1, awavfm, 
    -1, -1, giCosine, 1, 1, 1, ichannelmasks, 0, 
    giSine, giSine, giSine, giSine, 
    -1, asamplepos1, asamplepos1, asamplepos1, asamplepos1, 
    440, 440, 440, 440, 100, 1, giPanLaws

outch 1, a1, 2, a2, 3, a3, 4, a4
        
endin

</CsInstruments>
<CsScore>
i1 0 12
</CsScore>
</CsoundSynthesizer>
