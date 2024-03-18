<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o wgpluck2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

iplk = p4
kamp = .7
icps = 220
kpick = 0.75
krefl = p5

apluck wgpluck2 iplk, kamp, icps, kpick, krefl
apluck  dcblock2    apluck
outs apluck, apluck

endin

</CsInstruments>
<CsScore>
;         pluck   reflection
i 1 0 1     0       0.9
i 1 + 1     <       .
i 1 + 1     <       .
i 1 + 1     1       . 

i 1 5 5     .75     0.7 
i 1 + 5     .05     0.7 
e
</CsScore>
</CsoundSynthesizer>
