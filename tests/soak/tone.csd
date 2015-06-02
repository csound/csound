<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tone.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

asig diskin2 "beats.wav", 1
     outs asig, asig
endin

instr 2

kton line 10000, p3, 0		;all the way down to 0 Hz
asig diskin2 "beats.wav", 1
asig tone asig, kton		;half-power point at 500 Hz
     outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2

e

</CsScore>
</CsoundSynthesizer>
