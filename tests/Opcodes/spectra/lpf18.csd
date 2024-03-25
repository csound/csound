<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
;-o lpf18.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder with help from Iain Duncan

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
kamp init 1                         ; Note that its amplitude (kamp) ranges from 0 to 1.
kcps init 440
knh init 3
ifn = 1
asine buzz kamp, kcps, knh, ifn     ; Generate a sine waveform.
kfco line 300, p3, 3000             ; Filter the sine waveform.
kres init 0.8                       ; Vary the cutoff frequency (kfco) from 300 to 3,000 Hz.
kdist = p4
ivol = p5
aout lpf18 asine, kfco, kres, kdist
outs aout * ivol, aout * ivol

endin

</CsInstruments>
<CsScore>
f 1 0 16384 10 1    ; sine wave.

; different distortion and volumes to compensate
i 1 0 4     0.2         .8
i 1 4.5 4   0.9         .7
e
</CsScore>
</CsoundSynthesizer>
