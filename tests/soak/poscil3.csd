<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o poscil3.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

krnd  randomh 40, 440, 1	; produce random values
ain   poscil3 .6, krnd, giSine
kline line    1, p3, 0    	; straight line
aL,aR pan2    ain, kline	; sent across image
      outs    aL, aR

endin
</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>
