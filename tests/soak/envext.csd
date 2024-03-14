<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -m0  ;;;realtime audio out, with limiter protection
; For Non-realtime ouput leave only the line below:
; -o envext.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2022

instr 1
; analyzes sound file and outputs result to a file called "newenv"
ires1 system_i 1,{{ envext -w .05 Mathews.wav }}               
endin

</CsInstruments>
<CsScore>

i1 0 1  

e
</CsScore>
</CsoundSynthesizer>
