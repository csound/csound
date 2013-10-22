<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fold.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig  poscil3 .8, 400, 1 ;very clean sine
kincr line p4, p3, p5
asig  fold asig, kincr
      outs asig, asig

endin
</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0  4 2  2 
i 1 5  4 5  5 
i 1 10 4 10 10 
i 1 15 4 1 100	; Vary the fold-over amount from 1 to 100

e
</CsScore>
</CsoundSynthesizer>
