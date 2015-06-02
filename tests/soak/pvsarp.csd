<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     ;;;-d     RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsarp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 100
nchnls = 1
0dbfs = 1

instr 1
asig  in                                 ; get the signal in
idepth = p4

fsig  pvsanal   asig, 1024, 256, 1024, 1 ; analyse it
kbin  oscili   0.1, 0.5, 1               ; ftable 1 in the 0-1 range
ftps  pvsarp   fsig, kbin+0.01, idepth, 2   ; arpeggiate it (range 220.5 - 2425.5)
atps  pvsynth  ftps                      ; synthesise it

       out atps
endin


</CsInstruments>
<CsScore>
f 1 0 4096 10 1 ;sine wave

i 1 0 10 0.9
i 1 + 10 0.5
e


</CsScore>
</CsoundSynthesizer>
