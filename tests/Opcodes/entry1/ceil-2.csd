<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ceil-2.wav -W ;;; for file output any platform

; By Stefano Cucchi - 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps = 100
kcar = 1
kmod = p4

kndx oscil 30, .25/p3, 1
kndx ceil kndx

asig foscili .5, kcps, kcar, kmod, kndx, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>

f 1 0 16384 10 1

i 1 0 10 1.5	
e
</CsScore>
</CsoundSynthesizer>
