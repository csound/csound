<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o filebit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; Instrument #1.
instr 1
  ; Print out the number of channels in the 
  ; audio file "mary.wav".
  ibits filebit "mary.wav"
  print ibits
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for 1 second.
i 1 0 1
e


</CsScore>
</CsoundSynthesizer>
