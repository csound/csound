<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     ;;;-d     RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsarp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; additions by Menno Knevel 2021
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1
nchnls_i = 1  ; number of input channels

instr 1

asig  diskin2   "flute.aiff", 1, 0, 1      ; get the sample in
idepth = p4
prints "\n--**here the sample is used**--\n"
fsig  pvsanal   asig, 1024, 256, 1024, 1 ; analyse it
kbin  oscili   0.1, 0.05, 1               ; ftable 1 in the 0-1 range
ftps  pvsarp   fsig, kbin+0.01, idepth, 7   ; arpeggiate it (range 220.5 - 2425.5)
atps  pvsynth  ftps                      ; synthesise it
       outs atps, atps

endin

instr 2

asig  in                                 ; get the (microphone?) signal in
idepth = p4
prints "\n--**please use microphone**--\n"
prints "--**if no input is given, there will be only silence...\n"
fsig  pvsanal   asig, 1024, 256, 1024, 1 ; analyse it
kbin  oscili   0.1, 0.3, 1               ; ftable 1 in the 0-1 range
ftps  pvsarp   fsig, kbin+0.01, idepth, 7   ; arpeggiate it (range 220.5 - 2425.5)
atps  pvsynth  ftps                      ; synthesise it
       outs atps, atps

endin

</CsInstruments>
<CsScore>
f 1 0 4096 10 1 ;sine wave
s
i 1 0 10 0.5    ; notes for the flute sample
i 1 + 10 1
s
i 2 0 10 0.9    ; notes for the microphone
i 2 + 10 0.5
e
</CsScore>
</CsoundSynthesizer>
