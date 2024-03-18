<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o zawm.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Initialize the ZAK space.
; Create 1 a-rate variable and 1 k-rate variable.
zakinit 1, 1

; Instrument #1 -- a basic instrument.
instr 1
  ; Generate a simple sine waveform.
  asin oscil 15000, 440, 1

  ; Mix the sine waveform with za variable #1.
  zawm asin, 1
endin

; Instrument #2 -- another basic instrument.
instr 2
  ; Generate another waveform with a different frequency.
  asin oscil 15000, 880, 1

  ; Mix this sine waveform with za variable #1.
  zawm asin, 1
endin

; Instrument #3 -- generates audio output.
instr 3
  ; Read za variable #1, containing both waveforms.
  a1 zar 1

  ; Generate the audio output.
  out a1

  ; Clear the za variables, get them ready for 
  ; another pass.
  zacl 0, 1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1 for one second.
i 1 0 1
; Play Instrument #2 for one second.
i 2 0 1
; Play Instrument #3 for one second.
i 3 0 1
e


</CsScore>
</CsoundSynthesizer>
