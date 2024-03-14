<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n   ; no sound
; For Non-realtime ouput leave only the line below:
; -o filebit.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ibits filebit "stereoJungle.wav"
prints  "\nbit depth = %d bit\n\n", ibits

endin


</CsInstruments>
<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
