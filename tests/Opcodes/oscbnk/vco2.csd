<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o vco2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr      =  44100
ksmps   =  10
nchnls  =  1

; user defined waveform -1: trapezoid wave with default parameters (can be
; accessed at ftables starting from 10000)
itmp    ftgen 1, 0, 16384, 7, 0, 2048, 1, 4096, 1, 4096, -1, 4096, -1, 2048, 0
ift     vco2init -1, 10000, 0, 0, 0, 1
; user defined waveform -2: fixed table size (4096), number of partials
; multiplier is 1.02 (~238 tables)
itmp    ftgen 2, 0, 16384, 7, 1, 4095, 1, 1, -1, 4095, -1, 1, 0, 8192, 0
ift     vco2init -2, ift, 1.02, 4096, 4096, 2

        instr 1
kcps    expon p4, p3, p5                ; instr 1: basic vco2 example
a1      vco2 12000, kcps                ; (sawtooth wave with default
        out a1                          ; parameters)
        endin

        instr 2
kcps    expon p4, p3, p5                        ; instr 2:
kpw     linseg 0.1, p3/2, 0.9, p3/2, 0.1        ; PWM example
a1      vco2 10000, kcps, 2, kpw
        out a1
        endin

        instr 3
kcps    expon p4, p3, p5                ; instr 3: vco2 with user
a1      vco2 14000, kcps, 14            ; defined waveform (-1)
aenv    linseg 1, p3 - 0.1, 1, 0.1, 0   ; de-click envelope
        out a1 * aenv
        endin

        instr 4
kcps    expon p4, p3, p5                ; instr 4: vco2ft example,
kfn     vco2ft kcps, -2, 0.25           ; with user defined waveform
a1      oscilikt 12000, kcps, kfn       ; (-2), and sr/4 bandwidth
        out a1
        endin


</CsInstruments>
<CsScore>

i 1  0 3 20 2000
i 2  4 2 200 400
i 3  7 3 400 20
i 4 11 2 100 200

f 0 14

e


</CsScore>
</CsoundSynthesizer>
