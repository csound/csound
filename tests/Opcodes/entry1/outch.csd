<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o outch.wav -W ;;; for file output any platform

; By  Stefano Cucchi - 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig vco2 0.2, 100  ; Audio signal.   	
kout randomh 1, 2, 5  ; Extracts random number between 1 & 2 - 5 times per second.

    outch kout,asig ; Sends the signal to the channel according to "kout".

printks "signal is sent to channel %d\\n", .2, kout ; Prints the channel where you hear the sound.
endin

</CsInstruments>
<CsScore>

i 1 0 15
e
</CsScore>
</CsoundSynthesizer>
