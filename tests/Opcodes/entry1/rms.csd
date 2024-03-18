<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rms.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By Stefano Cucchi 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

areference diskin "fox.wav"
krms rms  areference       ; take the RMS of "fox.wav"
asound oscili krms, 440 ; use RMS as amplitude of sine wave     
outch 1, areference
outch 2, asound
endin


</CsInstruments>
<CsScore>
i 1 0 4
e
</CsScore>
</CsoundSynthesizer>
