<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o expsega.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Kevin Conder, Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

aenv expsega 0.01, 0.1, 1, 0.1, 0.01    ; Define a short percussive amplitude envelope
a1 oscili aenv, 440
outs a1, a1

endin

</CsInstruments>
<CsScore>
i 1 0 1
i 1 1 1
i 1 2 1
i 1 3 3
e
</CsScore>
</CsoundSynthesizer>
