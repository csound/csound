<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o delay.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Instrument #1 -- Silence on one channel
instr 1
  ; Make a basic sound.
  abeep vco    20000, 440, 1

  ; Delay the beep by 1 sample.
  idlt  =      1/sr
  adel  delay  abeep, idlt
  adel1 delay1 abeep 

  ; Send the beep to the left speaker and
  ; the difference in the delayes to the right speaker.
        outs   abeep, adel-adel1
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1.
i 1 0.0 1

e


</CsScore>
</CsoundSynthesizer>
