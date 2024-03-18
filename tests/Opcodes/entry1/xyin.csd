<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     --displays ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o xyin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 10
nchnls = 2

; Instrument #1.
instr 1
  ; Print and capture values every 0.1 seconds.
  iprd = 0.1
  ; The x values are from 1 to 30.
  ixmin = 1
  ixmax = 30
  ; The y values are from 1 to 30.
  iymin = 1
  iymax = 30
  ; The initial values for X and Y are both 15.
  ixinit = 15
  iyinit = 15

  ; Get the values kx and ky using the xyin opcode.
  kx, ky xyin iprd, ixmin, ixmax, iymin, iymax, ixinit, iyinit

  ; Print out the values of kx and ky.
  printks "kx=%f, ky=%f\\n", iprd, kx, ky

  ; Play an oscillator, use the x values for amplitude and
  ; the y values for frequency.
  kamp = kx * 1000
  kcps = ky * 220
  a1 poscil kamp, kcps, 1

  outs a1, a1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1 for 30 seconds.
i 1 0 30
e


</CsScore>
</CsoundSynthesizer>
