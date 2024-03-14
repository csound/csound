<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n  ; no sound
; For Non-realtime ouput leave only the line below:
; -o taninv.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

irad = 0.5
i1 = taninv(irad)
print i1

endin

</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
