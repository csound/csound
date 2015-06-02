<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o barmodel.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
  sr        =           44100
  kr        =           4410
  ksmps     =           10
  nchnls    =           1

; Instrument #1.
instr 1
  aq        barmodel    1, 1, p4, 0.001, 0.23, 5, p5, p6, p7
            out         aq
endin


</CsInstruments>
<CsScore>


i1 0.0 0.5  3 0.2 500  0.05
i1 0.5 0.5 -3 0.3 1000 0.05
i1 1.0 0.5 -3 0.4 1000 0.1
i1 1.5 4.0 -3 0.5 800  0.05
e
/* barmodel */

</CsScore>
</CsoundSynthesizer>
