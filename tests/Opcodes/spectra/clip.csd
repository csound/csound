<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o clip.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; white noise

arnd rand 1	; full amlitude
; Clip the noisy waveform's amplitude to 0.5
a1 clip arnd, p4, 0.5
   outs a1, a1

endin

instr 2	; white noise

arnd rand 1	; full amlitude
; Clip the noisy waveform's amplitude to 0.1
a1 clip arnd, p4, 0.1
   outs a1, a1

endin

</CsInstruments>
<CsScore>

; Play Instrument #1 for one second.
i 1 0 1 2
; Play Instrument #2 for one second.
i 2 1 1 2
s 3
; Play Instrument #1 for one second.
i 1 0 1 0
; Play Instrument #2 for one second.
i 2 1 1 0
s 3
; Play Instrument #1 for one second.
i 1 0 1 1
; Play Instrument #2 for one second.
i 2 1 1 1
e

</CsScore>
</CsoundSynthesizer>
