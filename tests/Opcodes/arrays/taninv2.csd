<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n   ; no sound
; For Non-realtime ouput leave only the line below:
; -o system.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments> 

;  by Kevin Conder

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
i1 taninv2 1, 2     ; Returns the arctangent for 1/2.
print i1

endin

</CsInstruments>
<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
