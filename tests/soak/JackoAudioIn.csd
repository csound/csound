<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o JackoAudioIn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr    	   = 48000 ; The control rate must be BOTH a power of 2 (for Jack)
ksmps 	   = 128
nchnls 	   = 2
0dbfs 	   = 1

; by Menno Knevel - 2023

JackoInit "default", "csound"
JackoAudioInConnect "system:capture_1", "audioin"   ; create 1 Audio input & connect from soundcard

instr 1     ; use Csound as an effect processor
asig    JackoAudioIn "audioin"
aout    nreverb	asig, 2, .3     
outs	aout, aout
endin

</CsInstruments>
<CsScore>

i 1 0 100
e
</CsScore>
</CsoundSynthesizer>
