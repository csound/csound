<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dam.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	;normal audio

asig diskin2 "beats.wav", 1, 0, 1
     outs asig, asig

endin


instr 2	; compressed audio

kthreshold = 0.2
icomp1 = 0.8
icomp2 = 0.2
irtime = 0.01
iftime = 0.5
asig diskin2 "beats.wav", 1, 0, 1
asig dam asig, kthreshold, icomp1, icomp2, irtime, iftime
    outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2.5 8.5

e
</CsScore>
</CsoundSynthesizer>