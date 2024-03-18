<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-n  ;;;no sound
; For Non-realtime ouput leave only the line below:
; -o pvsdisp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

asig soundin "fox.wav"  ;select a soundifle
fsig pvsanal asig, 1024, 256, 1024, 1
     pvsdisp fsig, 10

endin

</CsInstruments>
<CsScore>
i 1 0 2.7
e
</CsScore>
</CsoundSynthesizer>
