<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n   ; no sound
; For Non-realtime ouput leave only the line below:
; -o fprints.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Matt Ingalls, edited by Kevin Conder. 

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; a score generator
  
fprints "my.sco", "%!Generated score by ma++\\n \\n"    ; Print to the file "my.sco".

endin

</CsInstruments>
<CsScore>
i 1 0 0.001
e
</CsScore>
</CsoundSynthesizer>
