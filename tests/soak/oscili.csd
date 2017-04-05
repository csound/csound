<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o oscili.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kamp = .6
kcps = 440
ifn  = p4

asig oscil kamp, kcps, ifn
     outs asig,asig

endin

instr 2

kamp = .6
kcps = 440
ifn  = p4

asig oscili kamp, kcps, ifn
     outs asig,asig

endin
</CsInstruments>
<CsScore>
f1 0 128 10 1                                          ; Sine with a small amount of data
f2 0 128 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111   ; Sawtooth with a small amount of data
f3 0 128 10 1 0   0.3 0    0.2 0     0.14 0     .111   ; Square with a small amount of data
f4 0 128 10 1 1   1   1    0.7 0.5   0.3  0.1          ; Pulse with a small amount of data

i 1  0 2 1
i 2  3 2 1
i 1  6 2 2
i 2  9 2 2
i 1 12 2 3
i 2 15 2 3
i 1 18 2 4
i 2 21 2 4

e
</CsScore>
</CsoundSynthesizer>
