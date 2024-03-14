<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o JackoAudioOut.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr    	   = 48000 ; The control rate must be BOTH a power of 2 (for Jack)
ksmps 	   = 128
nchnls 	   = 2
0dbfs 	   = 1

; by Menno Knevel - 2023

JackoInit "default", "csound"
JackoAudioOutConnect  "audioout", "system:playback_4"   ; create 1 Audio output & connect to soundcard 

instr 1     
asig    vco2 .2, 100
JackoAudioOut "audioout", asig      ; signal is send to the 4th audio channel of the soundcard
asig2   vco2 .2, 40
outs asig2, asig2                   ; while asig2 is send to channel 1&2 of the soundcard
endin

</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
