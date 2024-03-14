<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cudsyth.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

i1 ftgen 1,0,512,7,1,512,0.001
i2 ftgen 2,0,512,-7,1,512,512
i3 ftgen 3,0,16384,10,1
schedule 1,0,10

instr 1
a1 cudasynth 0.001, 100,0, 2, 1,128
    out a1

endin
</CsInstruments>
<CsScore>
e 10
</CsScore>
</CsoundSynthesizer>

