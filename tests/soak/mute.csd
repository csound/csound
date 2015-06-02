<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mute.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; Mute Instrument #2.
mute 2
; Mute Instrument three.
mute "three"

instr 1

a1 oscils 0.2, 440, 0
   outs a1, a1
endin

instr 2	; gets muted

a1 oscils 0.2, 880, 0
   outs a1, a1
endin

instr three	; gets muted

a1 oscils 0.2, 1000, 0
   outs a1, a1
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 0 1
i "three" 0 1
e

</CsScore>
</CsoundSynthesizer>
