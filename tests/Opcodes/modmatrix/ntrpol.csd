<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ntrpol.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSin ftgen 1, 0, 1024, 10, 1

instr 1

avco vco2   .5, 110			;sawtootyh wave
asin poscil .5, 220, giSin		;sine wave but octave higher
kx   linseg 0, p3*.4, 1, p3*.6, 1	;crossfade between saw and sine
asig ntrpol avco, asin, kx
     outs   asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
