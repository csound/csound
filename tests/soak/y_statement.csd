<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o 0dbfs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

instr 1
  print p4
endin
</CsInstruments>
<CsScore>

y

i 1 0 1 [~ * 9 + 1]
i 1 + 1 [~ * 9 + 1]
i 1 + 1 [~ * 9 + 1]
i 1 + 1 [~ * 9 + 1]
e

</CsScore>
</CsoundSynthesizer>

