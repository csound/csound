<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o linseg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

kcps = cpspch(p4)
kenv linsegb 0, 0.25, 1, 1, 0
asig poscil kenv, kcps, giSine
     outs asig, asig

endin

instr 2	; scaling to duration

kcps = cpspch(p4)
kenv linseg 0, p3*0.25, 1, p3, 0
asig poscil kenv, kcps, giSine
     outs asig, asig

endin

</CsInstruments>
<CsScore>

i 1 0 1   7.00	; = 1 sec, p3 fits exactly
i 1 2 2   7.00	; = 2 sec, p3 truncated at 1 sec

i 2 4 1   7.00	; scales to duration
i 2 6 2   7.00	; of p3

e
</CsScore>
</CsoundSynthesizer>
