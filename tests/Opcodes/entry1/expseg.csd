<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
;-o expseg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder, additions by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
kcps = cpspch(p4)                           ; p4 = frequency in pitch-class notation.
kenv expseg 0.01, p3*0.25, 1, p3*0.75, 0.001 ; amplitude envelope.
kamp = kenv 
a1 oscili kamp, kcps
outs a1, a1

endin

</CsInstruments>
<CsScore>

i 1 0 0.5 8.00
i 1 1 0.5 8.01
i 1 2 0.5 8.02
i 1 3 2.0 8.03
e
</CsScore>
</CsoundSynthesizer>
