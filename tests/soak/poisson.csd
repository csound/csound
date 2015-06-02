<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o poisson.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 441  ;ksmps set deliberately high to have few k-periods per second
nchnls = 1

; Instrument #1.
instr 1
  ; Generates a random number in a poisson distribution.
  ; klambda = 1

  i1 poisson 1

  print i1
endin

instr 2

kres poisson p4
printk (ksmps/sr),kres ;prints every k-period
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for one second.
i 1 0 1
i 2 1 0.2 0.5
i 2 2 0.2 4   ;average 4 events per k-period
i 2 3 0.2 20  ;average 20 events per k-period
e


</CsScore>
</CsoundSynthesizer>
