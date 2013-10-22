<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o dcblock2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1	;add DC to "beats.wav"

asig soundin "beats.wav"
asig = asig+5000	;adds DC of 5000
     outs asig, asig
endin

instr 2	;dcblock audio

asig soundin "beats.wav"
asig = asig+5000	;adds DC
adc  dcblock2 asig	;remove DC again
     outs adc, adc

endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2
e

</CsScore>
</CsoundSynthesizer>
