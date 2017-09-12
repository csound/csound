<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lfo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps  = 5
itype = p4	;lfo type

klfo line 0, p3, 20
al   lfo klfo, kcps, itype
asig poscil .5, 220+al, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; sine wave.
f 1 0 32768 10 1

i 1 0 3 0	;lfo = sine
i 1 + 3 2	;lfo = square
i 1 + 3 5	;lfo = saw-tooth down
e
</CsScore>
</CsoundSynthesizer>
