<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o phaser2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

instr 2              ; demonstration of phase shifting abilities of phaser2. 
  ; Input mixed with output of phaser2 to generate notches. 
  ; Demonstrates the interaction of imode and ksep.
  idur   = p3 
  iamp   = p4 * .04
  iorder = p5        ; number of 2nd-order stages in phaser2 network
  ifreq  = p6        ; not used
  ifeed  = p7        ; amount of feedback for phaser2
  imode  = p8        ; mode for frequency scaling
  isep   = p9        ; used with imode to determine notch frequencies
  kamp   linseg 0, .2, iamp, idur - .2, iamp, .2, 0
  iharms = (sr*.4) / 100

  ; "Sawtooth" waveform exponentially decaying function, to control notch frequencies
  asig   gbuzz 1, 100, iharms, 1, .95, 2  
  kline  expseg 1, idur, .005
  aphs   phaser2 asig, kline * 2000, .5, iorder, imode, isep, ifeed

  out (asig + aphs) * iamp
endin


</CsInstruments>
<CsScore>

; cosine wave for gbuzz
f2 0  8192 9 1 1 .25     

; phaser2, imode=1
i2 00 10 7000 8 .2 .9 1 .33
i2 11 10 7000 8 .2 .9 1 2 

; phaser2, imode=2
i2 22 10 7000 8 .2 .9 2 .33
i2 33 10 7000 8 .2 .9 2 2
e


</CsScore>
</CsoundSynthesizer>
