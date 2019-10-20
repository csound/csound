<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac          ; -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o system.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

instr 1
; Waits for command to execute before continuing
  ires system_i 1, "echo hello world!"
  print ires
  turnoff
endin

instr 2
; Runs command in a separate thread
  ires system_i 1, "echo hello world!", 1
  print ires
  turnoff
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 1 1
e


</CsScore>
</CsoundSynthesizer>
