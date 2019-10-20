<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 4410
nchnls = 1

; Initialize the ZAK space.
; Create 3 a-rate variables and 5 k-rate variables.
zakinit 2, 3

instr 1 ;a simple waveform.
  ; Generate a simple sine waveform.
  asin oscil 20000, 440, 1

  ; Send the sine waveform to za variable #1.
  zaw asin, 1
endin

instr 2  ;generates audio output.
  ; Read za variable #1.
  a1 zar 1

  ; Generate audio output.
  out a1

  ; Clear the za variables, get them ready for
  ; another pass.
  zacl 0, 2
endin

instr 3  ;increments k-type channels
  k0 zkr 0
  k1 zkr 1
  k2 zkr 2

  zkw k0+1, 0
  zkw k1+5, 1
  zkw k2+10, 2
endin

</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

i 1 0 1
i 2 0 1

i 3 0 3
e

</CsScore>
</CsoundSynthesizer>
