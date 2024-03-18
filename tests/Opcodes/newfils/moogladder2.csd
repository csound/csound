<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o moogladder2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kfe  expseg 500, p3*0.9, 1800, p3*0.1, 3000
asig buzz  1, 100, 20, 1
kres line .1, p3, .99	;increase resonance
afil moogladder2 asig, kfe, kres
     outs afil, afil

endin
</CsInstruments>
<CsScore>
f 1 0 4096 10 1

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
