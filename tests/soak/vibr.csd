<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vibr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kaverageamp  init 500
kaveragefreq init 4
kvib vibr kaverageamp, kaveragefreq, 1
asig poscil .8, 220+kvib, 1		;add vibrato
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1       ;sine wave

i 1 0 10

e
</CsScore>
</CsoundSynthesizer>

