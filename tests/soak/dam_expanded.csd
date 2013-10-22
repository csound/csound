<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dam_expanded.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	

asig diskin2 "beats.wav", 1, 0, 1
     outs asig, asig

endin

instr 2	;expanded audio

kthreshold = .5
icomp1 = 2
icomp2 = 3
irtime = 0.01
iftime = 0.1

asig diskin2 "beats.wav", 1, 0, 1
asig dam asig, kthreshold, icomp1, icomp2, irtime, iftime
     outs asig*.2, asig*.2	;adjust volume of expanded beat

endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2.5 6.5

e
</CsScore>
</CsoundSynthesizer>