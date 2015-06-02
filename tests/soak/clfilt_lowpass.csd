<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o clfilt_lowpass.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; white noise

asig rand 0.5
outs asig, asig

endin

instr 2	; filtered noise 

asig rand 0.9
; Lowpass filter signal asig with a 
; 10-pole Butterworth at 500 Hz.
a1 clfilt asig, 500, 0, 10
   outs a1, a1

endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2
e

</CsScore>
</CsoundSynthesizer>
