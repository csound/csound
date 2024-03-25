<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o randomi.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

     seed 0
     
; Instrument #1.
instr 1
  ; Choose a random frequency between 220 and 440.
  ; Generate new random numbers at 10 Hz.
  kmin    init 220
  kmax    init 440
  kcps    init 10
  imode   =    p4
  ifstval =    p5
  
     printf_i "\nMode: %d\n", 1, imode
  k1 randomi kmin, kmax, kcps, imode, ifstval
     printks "k1 = %f\n", 0.1, k1
endin

</CsInstruments>
<CsScore>

; Play Instrument #1 for one second.
; each time with a different mode.
i 1 0 1
i 1 1 1 1
i 1 2 1 2 330
i 1 3 1 3
e

</CsScore>
</CsoundSynthesizer>

