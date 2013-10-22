<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o interp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 8000
kr = 8
ksmps = 1000
nchnls = 1

; Instrument #1 - a simple instrument.
instr 1
  ; Create an amplitude envelope.
  kamp linseg 0, p3/2, 20000, p3/2, 0

  ; The amplitude envelope will sound rough because it
  ; jumps every ksmps period, 1000.
  a1 oscil kamp, 440, 1
  out a1
endin

; Instrument #2 - a smoother sounding instrument.
instr 2
  ; Create an amplitude envelope.
  kamp linseg 0, p3/2, 25000, p3/2, 0
  aamp interp kamp

  ; The amplitude envelope will sound smoother due to
  ; linear interpolation at the higher a-rate, 8000.
  a1 oscil aamp, 440, 1
  out a1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 256 10 1

; Play Instrument #1 for two seconds.
i 1 0 2
; Play Instrument #2 for two seconds.
i 2 2 2
e


</CsScore>
</CsoundSynthesizer>
