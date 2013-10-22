<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fofilter.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kfe  expseg 10, p3*0.9, 180, p3*0.1, 175
kenv linen .1, 0.05, p3, 0.05
asig buzz  kenv, kfe, sr/(2*kfe), 1
afil fofilter asig, 900, 0.007, 0.04
     outs  afil, afil 

endin
</CsInstruments>
<CsScore>
; sine wave
f 1 0 16384 10 1

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
