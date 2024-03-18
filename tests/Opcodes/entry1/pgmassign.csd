<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in   No messages  MIDI in
-odac           -iadc     -d         -M0  ;;;RT audio I/O with MIDI in
; For Non-realtime ouput leave only the line below:
; -o pgmassign.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Program 55 (synth vox) uses Instrument #10.
pgmassign 55, 10 

; Instrument #10.
instr 10
  ; Just an example, no working code in here!
endin


</CsInstruments>
<CsScore>

; Play Instrument #10 for one second.
i 10 0 1
e


</CsScore>
</CsoundSynthesizer>
