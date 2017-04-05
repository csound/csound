<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o areson.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; unfiltered noise

asig rand 0.5		; white noise signal.
     outs asig, asig
endin

instr 2 ; filtered noise

kcf  init 1000
kbw  init 100
asig rand 0.5
afil areson asig, kcf, kbw
afil balance afil,asig 	; afil = very loud
     outs afil, afil
endin


</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2
e

</CsScore>
</CsoundSynthesizer>
