<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fmmetal.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kfreq = 440
kvdepth = 0
kvrate = 0
ifn1 = 1
ifn2 = 2
ifn3 = 2
ifn4 = 1
ivfn = 1
kc2  = p5

kc1  line p4, p3, 1
asig fmmetal .5, kfreq, kc1, kc2, kvdepth, kvrate, ifn1, ifn2, ifn3, ifn4, ivfn
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; sine wave.
f 1 0 32768 10 1
; the "twopeaks.aiff" audio file.
f 2 0 256 1 "twopeaks.aiff" 0 0 0 


i 1 0 4 6 5 
i 1 5 4 .2 10 
e
</CsScore>
</CsoundSynthesizer>
