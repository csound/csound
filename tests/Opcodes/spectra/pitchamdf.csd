<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pitchamdf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;clean audio

asig soundin p4
     outs asig, asig
endin

instr 2	;use pitch

Sfile = p4              
asig soundin Sfile

asig tone asig, 1000		;lowpass-filter
kcps, krms pitchamdf asig, 100, 500, 200
asig poscil krms, kcps, 1	;re-synthesize with sawtooth
printf  "now %s is used...\n\n", 1, Sfile
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1 0.5 0.3 0.25 0.2 0.167 0.14 0.125 .111   ;sawtooth

i 1 0   3 "fox.wav"
i 2 3   3 "fox.wav"
i 1 6   4 "singFemale.aif"
i 2 10  4 "singFemale.aif"
i 1 15  2 "drumsMlp.wav"
i 2 17  2 "drumsMlp.wav"
</CsScore>
</CsoundSynthesizer>
