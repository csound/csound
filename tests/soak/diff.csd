<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o diff.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

asig diskin2 "fox.wav", 1
     outs asig, asig

endin

instr 2	; with diff

asig diskin2 "fox.wav", 1
ares diff asig
     outs ares, ares

endin

instr 3	; with integ

asig diskin2 "fox.wav", 1
aint integ asig
aint = aint*.05			;way too loud
     outs aint, aint

endin

instr 4	; with diff and integ

asig diskin2 "fox.wav", 1
ares diff asig
aint integ ares
     outs aint, aint

endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 1 1
i 3 2 1
i 4 3 1

e

</CsScore>
</CsoundSynthesizer>
