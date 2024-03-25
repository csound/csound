<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o zakinit.wav -W ;;; for file output any platform
</CsOptions>
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

instr 4 ;displays values from k-type channels
  k0 zkr 0
  k1 zkr 1
  k2 zkr 2

  ; The total count for k0 is 30, since there are 10
  ; control blocks per second and intruments 3 and 4
  ; are on for 3 seconds.
  printf "k0 = %i\n",k0, k0
  printf "k1 = %i\n",k1, k1
  printf "k2 = %i\n",k2, k2
endin

</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

i 1 0 1
i 2 0 1

i 3 0 3
i 4 0 3
e

</CsScore>
</CsoundSynthesizer>
