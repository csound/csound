<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o wguide1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder
; additions by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; Generate some noise.
  
asig noise .5, 0.5
outs asig, asig

endin

instr 2     
  
asig noise .5, 0.5                          ; Generate some noise.
kfreq line p4, p3, 100                      ; Run it through a wave-guide model.
kcutoff init 3000
kfeedback init 0.8
awg1 wguide1 asig, kfreq, kcutoff, kfeedback
outs awg1, awg1

endin

</CsInstruments>
<CsScore>
i 1 0 2
;           freq
i 2 2 5     2000    ; falling frequency
i 2 8 3     100     ; static
e
</CsScore>
</CsoundSynthesizer>
