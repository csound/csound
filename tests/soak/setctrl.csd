<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o setctrl.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Instrument #1.
instr 1
  ; Display the label "Volume" on Slider #1.
  setctrl 1, "Volume", 4
  ; Set Slider #1's initial value to 20.
  setctrl 1, 20, 1
  
  ; Capture and display the values for Slider #1.
  k1 control 1
  printk2 k1

  ; Play a simple oscillator.
  ; Use the values from Slider #1 for amplitude.
  kamp = k1 * 128
  a1 oscil kamp, 440, 1
  out a1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1 for thirty seconds.
i 1 0 30
e


</CsScore>
</CsoundSynthesizer>
