<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fog.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

;p4 = transposition factor
;p5 = speed factor
;p6 = function table for grain data
i1    =	sr/ftlen(1) ;scaling to reflect sample rate and table length
a1    phasor i1*p5 ;index for speed
asigl fog    .5, 15, p4, a1, 1, 0, .01, .5, .01, 30, 1, 2, p3 		;left channel
asigr fog    .4, 25, p4+.2, a1, 1, 0, .01, .5, .01, 30, 1, 2, p3, .5	;right channel
      outs   asigl, asigr
endin

</CsInstruments>
<CsScore>
f 1 0 131072 1 "fox.wav" 0 0 0
f 2 0 1024 19 .5 .5 270 .5

i 1 0 10 .7  .1
i 1 + 4  1.2  2
e
</CsScore>
</CsoundSynthesizer>
