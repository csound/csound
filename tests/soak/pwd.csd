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
Swd pwd
puts Swd, 1
endin

</CsInstruments>
<CsScore>

; Play Instrument #1 for minimal time.
i 1 0 0.1
e


</CsScore>
</CsoundSynthesizer> 