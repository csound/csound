<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o insremot.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>
nchnls = 1

insremot "192.168.1.100", "192.168.1.101", 1

instr 1
  aq barmodel 1, 1, p4, 0.001, 0.23, 5, p5, p6, p7
     out      aq
endin

</CsInstruments>

<CsScore>
f0 360

e
</CsScore>

</CsoundSynthesizer>
