<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ftgen.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs   =1

gisine   ftgen 1, 0, 16384, 10, 1	;sine wave
gisquare ftgen 2, 0, 16384, 10, 1, 0 , .33, 0, .2 , 0, .14, 0 , .11, 0, .09 ;odd harmonics
gisaw    ftgen 3, 0, 16384, 10, 0, .2, 0, .4, 0, .6, 0, .8, 0, 1, 0, .8, 0, .6, 0, .4, 0,.2 ;even harmonics

instr 1

ifn = p4
asig poscil .6, 200, ifn
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 2 1 ;sine wave
i 1 3 2 2 ;odd harmonics
i 1 6 2 3 ;even harmonics
e
</CsScore>
</CsoundSynthesizer>
