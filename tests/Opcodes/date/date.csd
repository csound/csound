<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o date.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
instr 1
      ii,ij date
      print ii
      print ij
      Sa dates ii
      prints Sa
      Ss dates -1
      prints Ss
      St dates 1
      prints St 
endin

</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>

</CsoundSynthesizer>
