<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvstencil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

; By Stefano Cucchi - 2021

instr 1

fsource pvsdiskin "fox.pvx", 1, 0.2
kgain1  randomh p4, p5, p6
fstencil1  pvstencil fsource, kgain1, 1, 1
aout1   pvsynth   fstencil1
kgain2  randomh p7, p8, p9
fstencil2  pvstencil fsource, kgain2, 1, 1
aout2   pvsynth   fstencil2
        outs  aout1*0.2, aout2*0.2
endin

</CsInstruments>
<CsScore>
f1 0 513 -7 0 128 0.01 256 0 128 0.9
i1 0 10 4 70 4.2 12 95 3.8
e
</CsScore>
</CsoundSynthesizer>
