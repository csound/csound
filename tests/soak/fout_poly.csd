<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o fout_poly.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 44100
ksmps = 1
nchnls = 1

; Instrument #1 - Play an audio file.
instr 1
  ; Generate an audio signal using 
  ; the audio file "beats.wav".
  asig soundin "beats.wav"

  out asig
endin

; Instrument #2 - Create a basic tone.
instr 2
  iamp = 5000
  icps = 440
  iphs = 0

  ; Create an audio signal.
  asig oscils iamp, icps, iphs

  out asig
endin

; Instrument #99 - Save the global signal to a file.
instr 99
  ; Read the csound output buffer
  aoutput monitor
  ; Write the output of csound to a headerless 
  ; audio file called "fout_poly.raw".
  fout "fout_poly.raw", 1, aoutput

endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for two seconds.
i 1 0 2

; Play Instrument #2 every quarter-second.
i 2 0.00 0.1
i 2 0.25 0.1
i 2 0.50 0.1
i 2 0.75 0.1
i 2 1.00 0.1
i 2 1.25 0.1
i 2 1.50 0.1
i 2 1.75 0.1

; Make sure the global instrument, #99, is running
; during the entire performance (2 seconds).
i 99 0 2
e


</CsScore>
</CsoundSynthesizer>
