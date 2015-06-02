<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o equal.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 44100
ksmps = 1
nchnls = 1


instr 1
  ; Assign a value to the variable i1.
  i1 = 1234

  ; Print the value of the i1 variable.
  print i1

endin

</CsInstruments>

<CsScore>

; Play Instrument #1 for one second.
i 1 0 1
e

</CsScore>

</CsoundSynthesizer>

