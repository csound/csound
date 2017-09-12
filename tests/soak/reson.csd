<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o reson.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

asaw vco2 .2, 220	;sawtooth
kcf  line 220, p3, 1760	;vary cut-off frequency from 220 to 1280 Hz
kbw  = p4		;vary bandwidth of filter too		
ares reson asaw, kcf, kbw
asig balance ares, asaw
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 4 10	;bandwidth of filter = 10 Hz
i 1 + 4 50	;50 Hz and
i 1 + 4 200	;200 Hz
e
</CsScore>
</CsoundSynthesizer>
