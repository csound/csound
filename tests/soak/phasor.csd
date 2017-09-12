<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o phasor.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifn = 1			;read table 1 with our index
ixmode = 1
kndx phasor p4
kfrq table kndx, ifn, ixmode
asig poscil .6, kfrq, 2	;re-synthesize with sine
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 1025 -7 200 1024 2000 ;a line from 200 to 2,000	
f 2 0 16384 10 1;sine wave

i 1 0 1 1	;once per second
i 1 2 2 .5	;once per 2 seconds
i 1 5 1 2	;twice per second
e
</CsScore>
</CsoundSynthesizer>
