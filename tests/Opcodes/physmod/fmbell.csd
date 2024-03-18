<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fmbell.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kamp = p4
kfreq = 880
kc1 = p5
kc2 = p6
kvdepth = 0.005
kvrate = 6

asig fmbell kamp, kfreq, kc1, kc2, kvdepth, kvrate
     outs asig, asig
endin

instr 2

kamp = p4
kfreq = 880
kc1 = p5
kc2 = p6
kvdepth = 0.005
kvrate = 6

asig fmbell kamp, kfreq, kc1, kc2, kvdepth, kvrate, 1, 1, 1, 1, 1, p7
     outs asig, asig
endin

</CsInstruments>
<CsScore>
; sine wave.
f 1 0 32768 10 1

i 1 0 3 .2  5 5 
i 1 + 4 .3 .5 1
s
i 2 0 12 .2  5 5 16
i 2 + 12 .3 .5 1 12


e
</CsScore>
</CsoundSynthesizer>
