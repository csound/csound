<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pitch.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;clean audio

asig soundin p4
     outs asig, asig
endin

instr 2	;use pitch

iupdte = 0.001	;high definition
ilo = 6
ihi = 10
idbthresh = 10
ifrqs = 12
iconf = 10
istrt = 8

asig soundin p4
koct, kamp pitch asig, iupdte, ilo, ihi, idbthresh, ifrqs, iconf, istrt
kamp = kamp*.00005		;lower volume
kcps = cpsoct(koct)
asig poscil kamp, kcps, 1	;re-synthesize with sawtooth
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111   ;sawtooth

i 1 0  3 "fox.wav"
i 2 3  3 "fox.wav"
i 1 6  3 "mary.wav"
i 2 9  3 "mary.wav"
i 1 12 3 "beats.wav"
i 2 15 3 "beats.wav"
e
</CsScore>
</CsoundSynthesizer>
