<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac         -iadc       -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o zacl.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Initialize the ZAK space.
; Create 1 a-rate variable and 1 k-rate variable.
zakinit 1, 1

; Instrument #1 -- a simple waveform.
instr 1
  ; Generate a simple sine waveform.
  asin oscili 0.2, 440

  ; Declick
  asin *= linsegr(0, 0.05, 1, 0.05, 0)

  ; Send the sine waveform to za variable #1.
  zaw asin, 1
endin

; Instrument #2 -- generates audio output.
instr 2
  ; Send za location #1 to channel 1
  a1 zar 1
  out a1

  ; Clear the za variables, get them ready for 
  ; another pass.
  zacl 0, 1
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for one second.
i 1 0 1
; Play Instrument #2 until end of performance
i 2 0 -1
e 1.5

</CsScore>
</CsoundSynthesizer>
