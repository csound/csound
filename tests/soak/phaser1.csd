<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o phaser1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; demonstration of phase shifting abilities of phaser1.
instr 1
  ; Input mixed with output of phaser1 to generate notches.
  ; Shows the effects of different iorder values on the sound
  idur   = p3 
  iamp   = p4 * .05
  iorder = p5        ; number of 1st-order stages in phaser1 network.
                     ; Divide iorder by 2 to get the number of notches.
  ifreq  = p6        ; frequency of modulation of phaser1
  ifeed  = p7        ; amount of feedback for phaser1

  kamp   linseg 0, .2, iamp, idur - .2, iamp, .2, 0

  iharms = (sr*.4) / 100

  asig   gbuzz 1, 100, iharms, 1, .95, 2  ; "Sawtooth" waveform modulation oscillator for phaser1 ugen.
  kfreq  oscili 5500, ifreq, 1
  kmod   = kfreq + 5600

  aphs   phaser1 asig, kmod, iorder, ifeed

  out    (asig + aphs) * iamp
endin


</CsInstruments>
<CsScore>

; inverted half-sine, used for modulating phaser1 frequency
f1 0  16384 9 .5 -1 0
; cosine wave for gbuzz
f2 0  8192 9 1 1 .25

; phaser1
i1 0  5 7000 4  .2 .9
i1 6  5 7000 6  .2 .9
i1 12 5 7000 8  .2 .9
i1 18 5 7000 16 .2 .9
i1 24 5 7000 32 .2 .9
i1 30 5 7000 64 .2 .9
e


</CsScore>
</CsoundSynthesizer>
