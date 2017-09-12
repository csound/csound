<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o subinstr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Instrument #1 - Creates a basic tone.
instr 1
  ; Print the value of p4, should be equal to
  ; Instrument #2's iamp field.
  print p4

  ; Print the value of p5, should be equal to
  ; Instrument #2's ipitch field.
  print p5

  ; Create a tone.
  asig oscils p4, p5, 0

  out asig
endin


; Instrument #2 - Demonstrates the subinstr opcode.
instr 2
  iamp = 20000
  ipitch = 440

  ; Use Instrument #1 to create a basic sine-wave tone.
  ; Its p4 parameter will be set using the iamp variable.
  ; Its p5 parameter will be set using the ipitch variable.
  abasic subinstr 1, iamp, ipitch

  ; Output the basic tone that we have created.
  out abasic
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #2 for one second.
i 2 0 1
e


</CsScore>
</CsoundSynthesizer>
